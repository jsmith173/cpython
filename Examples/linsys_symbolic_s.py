from sympy import *

R1 = 1000.0
R2 = 2000.0
U  = 5.0

#Equations 
# Fi1/R1+(Fi1-U)/R2+(Fi1-Fi2)/R2 = 0
# Fi2/R1+(Fi2-U)/R1+(Fi2-Fi1)/R2 = 0

Fi1, Fi2 = symbols(['Fi1', 'Fi2'])
sol = solve([Fi1/R1+(Fi1-U)/R2+(Fi1-Fi2)/R2, Fi2/R1+(Fi2-U)/R1+(Fi2-Fi1)/R2], [Fi1, Fi2])
print(sol)

