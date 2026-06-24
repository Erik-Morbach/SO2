import matplotlib.pyplot as plt

bs = [16, 64, 256, 1024, 4096, 16384, 65536]
rates = [
	[27,26,27,27,27],
	[103, 98, 104, 104, 98],
	[353, 340, 362, 356, 357],
	[913, 848, 925, 890, 852],
	[1400, 1400, 977, 160, 358],
	[303, 411, 452, 311, 406],
	[343, 473, 381, 539, 399],
]

avg_rate = [sum(row)/len(row) for row in rates]
plt.plot(bs, avg_rate)
plt.show(