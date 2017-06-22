set xtics rotate font "times,5" 0,100,10000
set terminal pdf
set output 'zsrmv-test-plot.pdf'
plot 'ts-task1.txt' w i , 'ts-task2.txt' w i
