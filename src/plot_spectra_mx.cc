//  ********************************************************************
//  This file is part of KAT - the K-mer Analysis Toolkit.
//
//  KAT is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  KAT is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with KAT.  If not, see <http://www.gnu.org/licenses/>.
//  *******************************************************************

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sstream>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <iterator>
using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::stringstream;
using std::istringstream;
using std::ostringstream;
using std::ifstream;
using std::vector;

#include <boost/algorithm/string.hpp>
#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>
namespace po = boost::program_options;
namespace bfs = boost::filesystem;
using bfs::path;

#include "inc/gnuplot/gnuplot_i.hpp"
#include "inc/str_utils.hpp"

#include "inc/matrix/sparse_matrix.hpp"
#include "inc/matrix/matrix_metadata_extractor.hpp"

#include "plot_spectra_mx.hpp"

bool kat::PlotSpectraMx::plot() {

    // Check input file exists
    if (!bfs::exists(mxFile) && !bfs::symbolic_link_exists(mxFile)) {
        BOOST_THROW_EXCEPTION(PlotSpectraMxException() << PlotSpectraMxErrorInfo(string(
                    "Could not find matrix file at: ") + mxFile.string() + "; please check the path and try again.")); 
    }


    // Modify variables as appropriate
    string auto_title_x = mme::getString(mxFile, mme::KEY_X_LABEL);
    string auto_title_y = mme::getString(mxFile, mme::KEY_Y_LABEL);
    ostringstream auto_title_str;
    auto_title_str << auto_title_x << " vs " << auto_title_y;

    string t = !boost::equals(title, DEFAULT_PSMX_TITLE) ? title : auto_title_str.str();
    t = t.empty() ? DEFAULT_PSMX_TITLE : t;

    uint16_t x_range = xMax != DEFAULT_PSMX_X_MAX ? xMax : mme::getNumeric(mxFile, mme::KEY_NB_COLUMNS);
    uint64_t y_range = yMax != DEFAULT_PSMX_Y_MAX ? yMax : DEFAULT_PSMX_Y_MAX;


    // Initialise gnuplot
    Gnuplot spectra_mx_plot = Gnuplot("lines");

    spectra_mx_plot.configurePlot(outputType, output.string(), width, height);

    spectra_mx_plot.set_title(t);
    spectra_mx_plot.set_xlabel(xLabel);
    spectra_mx_plot.set_ylabel(yLabel);
    spectra_mx_plot.set_xrange(xLogscale ? 1 : xMin, x_range);
    spectra_mx_plot.set_yrange(yLogscale ? 1 : yMin, y_range);

    if (xLogscale)
        spectra_mx_plot.set_xlogscale();
    if (yLogscale)
        spectra_mx_plot.set_ylogscale();

    //spectra_mx_plot.cmd("set size ratio 1");
    spectra_mx_plot.cmd("set key font \",8\"");
    spectra_mx_plot.cmd("set xlabel offset \"0,1\" font \",10\"");
    spectra_mx_plot.cmd("set ylabel offset \"2,0\" font \",10\"");
    spectra_mx_plot.cmd("set title font \",10\"");
    spectra_mx_plot.cmd("set tics font \", 8\"");
    spectra_mx_plot.cmd("set palette rgb 33,13,10");
    spectra_mx_plot.cmd("unset colorbox");

    spectra_mx_plot.cmd("set style data linespoints");

    ostringstream plot_str;

    string data;

    if (!list.empty())
    {
        if (verbose)
            cerr << "Extracting requested data from matrix... " << endl;

        data = getDataFromList(mxFile, list);

        if (verbose)
            cerr << "done." << endl;
    }
    else if (intersection)
    {
        if (verbose)
            cerr << "Extracting Intersection data from matrix... ";

        data = getIntersectionData(mxFile, excCutoffD1, excCutoffD2);

        if (verbose)
            cerr << "done." << endl;
    }
    else
    {
        BOOST_THROW_EXCEPTION(PlotSpectraMxException() << PlotSpectraMxErrorInfo(string(
                "Error: not sure how to process matrix.  You did not select a list of content from the matrix (\"--list\"), or alternatively select intersection mode (\"--intersection\")")));
    }


    plot_str << "plot " << data ;

    if (!spectra_mx_plot.is_valid()) {
        return false;
    }
    
    spectra_mx_plot.cmd(plot_str.str());

    if (verbose)
        //cerr << "Data Plotted." << endl;
        cerr << "Plotted data: " << plot_str.str() << endl;
    
    return true;
}

       
string kat::PlotSpectraMx::getDataFromList(const path& mx_file, string list)
{
    ostringstream data_str;

    // Split string by commas
    vector<string> parts = kat::splitString(list, ',');

    // Need to change colors and labels
    for(uint16_t i = 0; i < parts.size(); i++)
    {
        data_str << "'-' using 1:2 with linespoints ps 0.25 linetype 1 linecolor " << i+1 << " title '" << parts[i] << "'";
        if (i != parts.size() - 1)
        {
            data_str << ", ";
        }
    }

    data_str << "\n";

    SparseMatrix<uint64_t> mx = SparseMatrix<uint64_t>(mx_file);

    for(uint16_t i = 0; i < parts.size(); i++)
    {
        char c_or_r = parts[i][0];
        uint16_t index = atoi(parts[i].substr(1).c_str());
        ostringstream convert; convert << index; string index_str = convert.str();

        if (parts[i].substr(1).compare(index_str) != 0) {
            BOOST_THROW_EXCEPTION(PlotSpectraMxException() << PlotSpectraMxErrorInfo(string(
                    "Your row or column index is not valid.")));                                        
        }


        if (c_or_r == 'c')
        {
            for(uint16_t j = 0; j < mx.width(); j++)
            {
                data_str << j << " " << mx.get(index,j) << endl;
            }
        }
        else if (c_or_r == 'r')
        {
            for(uint16_t j = 0; j < mx.height(); j++)
            {
                data_str << j << " " << mx.get(j,index) << endl;
            }
        }
        else
        {
            BOOST_THROW_EXCEPTION(PlotSpectraMxException() << PlotSpectraMxErrorInfo(string(
                    "Unrecognised list item identifier.  Expected 'c' or 'r'."))); 
        }

        data_str << "e\n";
    }

    return data_str.str();
}


string kat::PlotSpectraMx::getIntersectionData(const path& mx_file, uint16_t exc_cutoff_d1, uint16_t exc_cutoff_d2)
{
    // Ready buffer for data
    ostringstream data_str;

    // Load matrix
    SparseMatrix<uint64_t> mx(mx_file);

    cerr << "Matrix loaded:- Width: " << mx.width() << "; Height: " << mx.height() << ";" << endl;

    // Define datasets
    data_str << "'-' using 1:2 with linespoints ps 0.25 linetype 1 linecolor " << "1" << " title '" << "dataset 1 exclusive content" << "',";
    data_str << "'-' using 1:2 with linespoints ps 0.25 linetype 1 linecolor " << "2" << " title '" << "dataset 1 shared content" << "',";
    data_str << "'-' using 1:2 with linespoints ps 0.25 linetype 1 linecolor " << "3" << " title '" << "dataset 2 exclusive content" << "',";
    data_str << "'-' using 1:2 with linespoints ps 0.25 linetype 1 linecolor " << "4" << " title '" << "dataset 2 shared content" << "'" << endl;

    cerr << "Datasets defined" << endl;


    // Acquire data

    // Dataset 1 Exclusive content
    for(uint16_t j = exc_cutoff_d1; j < mx.width(); j++)
    {
        uint64_t sum = mx.sumColumn(j, 0, exc_cutoff_d2 - 1);
        data_str << j << " " << sum << endl;
    }
    data_str << "e\n";
    cerr << "Dataset 1 Exclusive content collected" << endl;


    // Dataset 1 Shared content
    for(uint16_t j = exc_cutoff_d1; j < mx.width(); j++)
    {
        uint64_t sum = mx.sumColumn(j, exc_cutoff_d2, mx.height() - 1);
        data_str << j << " " << sum << endl;
    }
    data_str << "e\n";
    cerr << "Dataset 1 Shared content calculated" << endl;

    // Dataset 2 Exclusive content
    for(uint16_t j = exc_cutoff_d2; j < mx.height(); j++)
    {
        uint64_t sum = mx.sumRow(j, 0, exc_cutoff_d1 - 1);
        data_str << j << " " << sum << endl;
    }
    data_str << "e\n";
    cerr << "Dataset 2 Exclusive content collected" << endl;


    // Dataset 2 Shared content
    for(uint16_t j = exc_cutoff_d2; j < mx.height(); j++)
    {
        uint64_t sum = mx.sumRow(j, exc_cutoff_d1, mx.width() - 1);
        data_str << j << " " << sum << endl;
    }
    data_str << "e\n";
    cerr << "Dataset 2 Shared content calculated" << endl;

    return data_str.str();
}
       
int kat::PlotSpectraMx::main(int argc, char *argv[]) {



    path        mx_file;           
    string      output_type;
    path        output;
    string      title;
    string      x_label;
    string      y_label;
    uint16_t    width;
    uint16_t    height;
    bool        intersection;
    string      list;
    uint16_t    exc_cutoff_d1;
    uint16_t    exc_cutoff_d2;
    uint32_t    x_min;
    uint32_t    y_min;
    uint32_t    x_max;
    uint64_t    y_max;
    bool        x_logscale;
    bool        y_logscale;
    bool        verbose;
    bool        help;

    // Declare the supported options.
    po::options_description generic_options(PlotSpectraMx::helpMessage(), 100);
    generic_options.add_options()
            ("output_type,p", po::value<string>(&output_type)->default_value("png"), 
                "The plot file type to create: png, ps, pdf.  Warning... if pdf is selected please ensure your gnuplot installation can export pdf files.")
            ("output,o", po::value<path>(&output),
                "The path to the output file")
            ("title,t", po::value<string>(&title)->default_value(DEFAULT_PSMX_TITLE),
                "Title for plot")
            ("x_label,a", po::value<string>(&x_label)->default_value(DEFAULT_PSMX_X_LABEL),
                "Label for the x-axis (value taken from matrix metadata if present)")
            ("y_label,b", po::value<string>(&y_label)->default_value(DEFAULT_PSMX_Y_LABEL),
                "Label for the y-axis (value taken from matrix metadata if present)")
            ("x_min,r", po::value<uint32_t>(&x_min)->default_value(0),
                "Minimum value for the x-axis.")
            ("y_min,s", po::value<uint32_t>(&y_min)->default_value(0),
                "Minimum value for the y-axis.")
            ("x_max,x", po::value<uint32_t>(&x_max)->default_value(DEFAULT_PSMX_X_MAX),
                "Maximum value for the x-axis (value taken from matrix metadata if present)")
            ("y_max,y", po::value<uint64_t>(&y_max)->default_value(DEFAULT_PSMX_Y_MAX),
                "Maximum value for the y-axis (value taken from matrix metadata if present)")
            ("width,w", po::value<uint16_t>(&width)->default_value(1024),
                "Width of canvas")
            ("height,h", po::value<uint16_t>(&height)->default_value(1024),
                "Height of canvas")
            ("intersection,n", po::bool_switch(&intersection)->default_value(false),
                "Activate intersection mode, which plots the shared and exclusive content found in the matrix.")
            ("list,c", po::value<string>(&list),
                "The list of columns or rows to select from the matrix.  Note that this option will override \"--intersection\" if that was also selected.")
            ("exc_cutoff_d1,e", po::value<uint16_t>(&exc_cutoff_d1)->default_value(1),
                "If in \"--intersection\" mode, this enables you to alter the level at which content for dataset 1 is considered exclusive or shared.")
            ("exc_cutoff_d2,f", po::value<uint16_t>(&exc_cutoff_d2)->default_value(1),
                "If in \"--intersection\" mode, this enables you to alter the level at which content for dataset 2 is considered exclusive or shared.")
            ("x_logscale,l", po::bool_switch(&x_logscale)->default_value(false),
                "X-axis is logscale.  This overrides the x_min and x_max limits.")
            ("y_logscale,m", po::bool_switch(&y_logscale)->default_value(false),
                "Y-axis is logscale.  This overrides the y_min and y_max limits.")
            ("verbose,v", po::bool_switch(&verbose)->default_value(false), 
                "Print extra information.")
            ("help", po::bool_switch(&help)->default_value(false), "Produce help message.")
            ;

    // Hidden options, will be allowed both on command line and
    // in config file, but will not be shown to the user.
    po::options_description hidden_options("Hidden options");
    hidden_options.add_options()
            ("mx_file,s", po::value<path>(&mx_file), "Path to the matrix file to plot.")                    
            ;

    // Positional option for the input bam file
    po::positional_options_description p;
    p.add("mx_file", 1);


    // Combine non-positional options
    po::options_description cmdline_options;
    cmdline_options.add(generic_options).add(hidden_options);

    // Parse command line
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(cmdline_options).positional(p).run(), vm);
    po::notify(vm);

    // Output help information the exit if requested
    if (help || argc <= 1) {
        cout << generic_options << endl;
        return 1;
    }
    
    if (output.empty()) {
        BOOST_THROW_EXCEPTION(PlotSpectraMxException() << PlotSpectraMxErrorInfo(string(
            "Output file not specified.  Please use the '-o' option."))); 
    }
    

    PlotSpectraMx psmx(mx_file, output);
    psmx.setExcCutoffD1(exc_cutoff_d1);
    psmx.setExcCutoffD2(exc_cutoff_d2);
    psmx.setHeight(height);
    psmx.setIntersection(intersection);
    psmx.setList(list);
    psmx.setOutputType(output_type);
    psmx.setTitle(title);
    psmx.setVerbose(verbose);
    psmx.setWidth(width);
    psmx.setXLabel(x_label);
    psmx.setXLogscale(x_logscale);
    psmx.setXMax(x_max);
    psmx.setXMin(x_min);
    psmx.setYLabel(y_label);
    psmx.setYLogscale(y_logscale);
    psmx.setYMax(y_max);
    psmx.setYMin(y_min);

    psmx.plot();

    return 0;
}
