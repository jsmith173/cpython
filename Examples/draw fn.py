import math

#Keep the program structure, variable names (draw_pref, curves, curves_prefs)
curves = []
curves_prefs = []

#control data
draw_pref = {'typ': 0, 'l_limit': 0.0, 'r_limit': 50.0, 'i_subdiv': 1000, 
 'u_par': 's', 'u_res': 'V', 
 'n_par': 't', 'n_res': 'Out', 
 'pagename': 'Test',
 'flags': 0, 'curvecolor': 0, 'curvewidth': 1}

#step function
def frange(start, stop=None, step=None):
    count = 0
    while True:
        temp = float(start + count * step)
        if temp >= stop:
            break
        yield temp
        count += 1

#fn1: calculate values ( sin(x) )
def fn1_create_curve():
	values = []
	step = (draw_pref['r_limit']-draw_pref['l_limit'])/draw_pref['i_subdiv'];
	for x in frange(draw_pref['l_limit'], draw_pref['r_limit'], step):
	
		#type your function here
		y = math.sin(x)
		
		values.append(y)
	return values

#fn2: calculate values ( triangle wave, user defined )
def fn2_create_curve():
	values = []
	step = (draw_pref['r_limit']-draw_pref['l_limit'])/draw_pref['i_subdiv'];
	T = 20
	for x in frange(draw_pref['l_limit'], draw_pref['r_limit'], step):
	
		#type your function here
		x -= int(x/T)*T
		if x <= T/4:
			y = x
		elif x > T/4 and x <= 3*T/4:
			y = -x+T/2
		elif x > 3*T/4 and x <= T:
			y = x-T		
		
		values.append(y)
	return values
	
mycurve1 = fn1_create_curve()
curves.append(mycurve1)
curves_prefs.append(draw_pref)

mycurve2 = fn2_create_curve()
curves.append(mycurve2)
curves_prefs.append(draw_pref)

