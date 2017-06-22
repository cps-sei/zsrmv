set xtics rotate font "times,3" 0,100,10000
set terminal pdf
set output 'plot-five.pdf'
plot 'ts-task0.txt' w i , 'ts-task1.txt' w i, 'ts-task2.txt' w i, 'ts-task3.txt' w i, 'ts-task4.txt' w i
