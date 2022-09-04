import math

#Keep the program structure, variable names (curves, curves_prefs)
curves = []
curves_prefs = []

#control data
draw_pref1 = {'typ': 2, 'l_limit': 10.0, 'r_limit': 1e6, 'i_subdiv': 100, 
 'u_par': 'Hz', 'u_res': '', 
 'n_par': 's', 'n_res': 'VOut', 
 'fname': 'mycurve1'}

#step function
def frange(l, h, np):
    p = 0
    while True:
        temp = float(start + count * step)
		temp = l * (10 ** (math.log10(h / l) / np * p))
        if temp >= h:
            break
        yield temp
        p += 1

#fn1: calculate values ( AC, user defined )
def fn1_create_curve():
	values = []
	for x in frange(draw_pref1['l_limit'], draw_pref1['r_limit'], draw_pref1['i_subdiv']-1):
	
		#type your function here
		y = math.sin(x)
		
		values.append(y)
	return values

mycurve1 = fn1_create_curve()
curves.append(mycurve1)
curves_prefs.append(draw_pref1)

