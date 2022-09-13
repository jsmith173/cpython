import eseries

x = eseries.find_nearest(eseries.E12, 4.0)
print("E12(4) = %.3f" %x)

x = eseries.find_nearest(eseries.E24, 5.0)
print("E24(5) = %.3f" %x)

x = eseries.find_nearest(eseries.E48, 6.0)
print("E48(6) = %.3f" %x)

x = eseries.find_nearest(eseries.E96, 7.0)
print("E96(7) = %.3f" %x)
