import csv

fname_csv = 'data/codes.csv'
fname_src = 'CLErrorLookup/src/get_error_info.cpp'

# retrieve code infos from the csv file
data = []

with open(fname_csv, newline='') as f:
    reader = csv.reader(f, delimiter=',')
    next(reader, None)  # skip the headers

    for row in reader:
        data.append(row)

# generate the C++ source file
STUB = '''/* Copyright (c) 2024 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include "cl_error_lookup.hpp"

namespace clerror
{

ErrorInfo get_error_info(int code)
{
  ErrorInfo error_info;

  switch(code)
    {
// clang-format off
##SRC##
// clang-format on
    default:
      break;
    }

  return error_info;
}

}
'''

STUB_CASE = '''
    case ##CODE##:
      {
	error_info.code = code;
	error_info.flag = "##FLAG##";
	error_info.description = "##DESCRIPTION##";
	error_info.type = "##TYPE##";
	}
	break;
'''

src = ''

for row in data:
    src += STUB_CASE.replace('##CODE##', row[0]).replace('##FLAG##', row[1]).replace('##DESCRIPTION##', row[3].replace('‘', '')).replace('##TYPE##', row[4])

with open(fname_src, 'w') as f:
    f.write(STUB.replace('##SRC##', src))
