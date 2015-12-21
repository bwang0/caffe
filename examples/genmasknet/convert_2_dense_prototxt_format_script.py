#!/usr/bin/env python

import os, sys

#class dense_proto:
def handle_line(line, dense_str):
  if line.find('#') != -1 or len(line.strip())==0:
    if dense_str[-1] == "\n":
      return line
    else:
      return "\n"+line

  if line.find(':') != -1:
    return line.strip()+" "
  if line.find('{') != -1:
    return "\n"+line.rstrip()+' '
  if line.find('}') != -1:
    if dense_str.rstrip()[-1] == "}":
      return "\n"+line.rstrip()
    else:
      return line.strip()



if __name__ == "__main__":
  if len(sys.argv) != 3:
    print("./convert_2_dense_prototxt_format.py <input_file> <output_file>")
  if sys.argv[1] == sys.argv[2]:
    print("Error: For safety reasons, input_file can not be the same as output_file")
    sys.exit(1)

  print("Converting......")
  f = open(sys.argv[1])
  dense_f = open(sys.argv[2], 'w')
  dense_str = ""

  for line in f:
    #print handle_line(line, dense_str)
    dense_str += handle_line(line, dense_str)

  dense_f.write(dense_str)

  f.close()
  dense_f.close()