from mathmatrix import Matrix
 
R1 = 1000
R2 = 2000
U  = 5

#Equations 
# Fi1/R1+(Fi1-U)/R2+(Fi1-Fi2)/R2 = 0
# Fi2/R1+(Fi2-U)/R1+(Fi2-Fi1)/R2 = 0
 
# Coefficients (first equation)
#Fi1: 1/R1+2/R2 
#Fi2: -1/R2
#b0: U/R2

# Coefficients (second equation)
#Fi1: -1/R2
#Fi2: 2/R1+1/R2
#b1: U/R1

# Ax = b
# x = invA*b

list = [[1/R1+2/R2, -1/R2], [-1/R2, 2/R1+1/R2]]
list_b = [[U/R2], [U/R1]]

A = Matrix(2, 2, list)
b = Matrix(2, 1, list_b)
invA = A.inverse()
x = invA*b
print('Fi1 is: %0.4f' % (x[0][0]))
print('Fi2 is: %0.4f' % (x[1][0]))

