#! /usr/bin/python3
import math

def calc_if_times(x, k):
  res =  x // k
  if x % k == 0:
    return res
  return res + math.ceil(math.log2(k))

print(calc_if_times(64, 11))

res = []

for i in range(1, 40):
  sum = 0
  for j in range(65):
    sum += calc_if_times(j, i)
  res.append(sum)

print(res)