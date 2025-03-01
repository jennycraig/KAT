#!/usr/bin/env python

import argparse
import numpy as np
import matplotlib.pyplot as plt

# ----- command line parsing -----
parser = argparse.ArgumentParser(
    description="Creates K-mer spectra plot from selected rows and/or columns in a \"comp\" matrix.")

parser.add_argument("mx_file", type=str,
                    help="The input mx file from KAT")

parser.add_argument("-p", "--output_type", type=str, default="png",
                    choices=["png", "pdf", "eps"],
                    help="The plot file type to create.")
parser.add_argument("-o", "--output", type=str, default="kat-spectra-mx",
                    help="The path to the output file.")
parser.add_argument("-t", "--title", type=str, default="Spectra MX Plot",
                    help="Title for plot")
parser.add_argument("-a", "--x_label", type=str, default="X",
                    help="Label for x-axis")
parser.add_argument("-b", "--y_label", type=str, default="Y",
                    help="Label for y-axis")
parser.add_argument("-r", "--x_min", type=int, default=0,
                    help="Minimum value for x-axis")
parser.add_argument("-s", "--y_min", type=int, default=0,
                    help="Minimum value for y-axis")
parser.add_argument("-x", "--x_max", type=int, default=1000,
                    help="Maximum value for x-axis")
parser.add_argument("-y", "--y_max", type=int, default=1000,
                    help="Maximum value for y-axis")
parser.add_argument("-w", "--width", type=int, default=1024,
                    help="Width of canvas")
parser.add_argument("-l", "--height", type=int, default=1024,
                    help="Height of canvas")
parser.add_argument("-i", "--intersection", dest="intersection", action="store_true",
                    help="Activate intersection mode, which plots the shared and exclusive content found in the matrix.")
parser.set_defaults(intersection=False)
parser.add_argument("-c", "--list", type=str,
                    help="The list of columns or rows to select from the matrix (overrides -i)")
parser.add_argument("-e", "--exc_cutoff_d1", type=int, default=1,
                    help="If in intersection mode, the level at which content for dataset 1 is considered exclusive or shared")
parser.add_argument("-f", "--exc_cutoff_d2", type=int, default=1,
                    help="If in intersection mode, the level at which content for dataset 2 is considered exclusive or shared")
parser.add_argument("-m", "--x_logscale", dest="x_logscale", action="store_true",
                    help="X-axis is logscale. Overrides x_min and x_max")
parser.set_defaults(x_logscale=False)
parser.add_argument("-n", "--y_logscale", dest="y_logscale", action="store_true",
                    help="Y-axis is logscale. Overrides y_min and y_max")
parser.set_defaults(y_logscale=False)
parser.add_argument("-v", "--verbose", dest="verbose", action="store_true",
                    help="Print extra information")
parser.set_defaults(verbose=False)

args = parser.parse_args()
# ----- end command line parsing -----

