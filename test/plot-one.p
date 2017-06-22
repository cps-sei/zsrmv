set xtics rotate font "times,5" 0,100,10000
set terminal pdf
set output 'zsrmv-one-test-plot.pdf'
plot 'ts-task.txt' w i 
