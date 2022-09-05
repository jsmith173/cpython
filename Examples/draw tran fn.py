import math

#Keep the program structure, variable names (curves, curves_prefs)
curves = []
curves_prefs = []

#control data
draw_pref1 = {'typ': 0, 'l_limit': 0.0, 'r_limit': 50.0, 'i_subdiv': 1000, 
 'u_par': 's', 'u_res': 'V', 
 'n_par': 't', 'n_res': 'Out', 
 'pagename': 'Test', 'fname': 'mycurve1',
 'flags': 0, 'curvecolor': 0, 'curvewidth': 1}
draw_pref2 = {'typ': 0, 'l_limit': 0.0, 'r_limit': 50.0, 'i_subdiv': 1000, 
 'u_par': 's', 'u_res': 'V', 
 'n_par': 't', 'n_res': 'Out', 
 'pagename': 'Test', 'fname': 'mycurve2',
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
	step = (draw_pref1['r_limit']-draw_pref1['l_limit'])/draw_pref1['i_subdiv'];
	for x in frange(draw_pref1['l_limit'], draw_pref1['r_limit'], step):
	
		#type your function here
		y = math.sin(x)
		
		pair = [x, y]	
		values.append(pair)
	return values

#fn2: calculate values ( triangle wave, user defined )
def fn2_create_curve():
	values = []
	step = (draw_pref2['r_limit']-draw_pref2['l_limit'])/draw_pref2['i_subdiv'];
	T = 20
	for x in frange(draw_pref2['l_limit'], draw_pref2['r_limit'], step):
	
		#type your function here
		x0 = x; x0 -= int(x/T)*T
		if x0 <= T/4:
			y = x0
		elif x0 > T/4 and x0 <= 3*T/4:
			y = -x0+T/2
		elif x0 > 3*T/4 and x0 <= T:
			y = x0-T		
		
		pair = [x, y]
		values.append(pair)
	return values
	
mycurve1 = fn1_create_curve()
curves.append(mycurve1)
curves_prefs.append(draw_pref1)

mycurve2 = fn2_create_curve()
curves.append(mycurve2)
curves_prefs.append(draw_pref2)

