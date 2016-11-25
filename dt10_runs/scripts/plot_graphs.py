#!/bin/python

import json
import sys
import os
import math
import random
import matplotlib

matplotlib.use('Agg')


import matplotlib.pyplot as plt
import re
import numpy as np
import sqlite3



con = sqlite3.connect("all.db")

names=set( [ x[0] for x in con.execute("select name from rows").fetchall()] )
print(names)

opt_in_names=set( [ x[0] for x in con.execute("select name from rows where opt_in='1'").fetchall()] )
print(opt_in_names)

puzzles=set( [ x[0] for x in con.execute("select puzzle from rows").fetchall()] )
print(puzzles)

runs=set( [ x[0] for x in con.execute("select run from rows").fetchall()] )
print(runs)

#print( [x for x in con.execute("select * from rows").fetchall()])

def add_scaling(ax):
    xmin,xmax=ax.get_xlim()
    ax.set_xlim(xmin,xmax)
    ymin,ymax=ax.get_ylim()
    ax.set_ylim(ymin,ymax)

    x=np.logspace(math.log10(xmin),math.log10(xmax))

    s=1e-8
    while s < 1e8:
        y=x*s
        ax.plot(x,y,'gray', linestyle=':')
        y=(x*s)**2
        ax.plot(x,y,'gray', linestyle=':')
        y=(x*s)**3
        ax.plot(x,y,'gray', linestyle=':')
        s=s*10

def plot_perf_for_puzzle(puzzle,selName,run):
    plt.clf()
    plt.cla()
    ax=plt.gca()
    ax.set_yscale('log')
    ax.set_xscale('log')

    for name in names:
        data=con.execute("select n,currTime from rows where puzzle='"+puzzle+"' and name='"+name+"' and run='"+run+"' and opt_in='1' order by n").fetchall();
        style="-"# if selName==name else "--"
        lineWidth=2.0 if selName==name else 1.0
        marker="o" if selName==name else " "
        zorder=2 if selName==name else 0
        alpha=1.0 if selName==name else 0.5
        ax.plot( [x[0] for x in data], [x[1]/1000.0 for x in data], style, label=name, linewidth=lineWidth, marker=marker, zorder=zorder, alpha=alpha )


    add_scaling(ax)

    ax.set_ylabel('Performance (seconds)')
    ax.set_xlabel('Problem size (n)')
    #ax.legend(loc='upper left')
    plt.title('Execution time against problem size, puzzle={0}, user={1}, run={2}'.format(puzzle,selName,run))
    #plt.show()

#plot_perf_for_puzzle("median_bits", "ajw10_ml3411")
#plt.show()


def plot_speedup_for_puzzle_vs_median(puzzle,selName,run):
    plt.clf()
    plt.cla()
    ax=plt.gca()

    base=con.execute("select n,group_concat(currTime) from rows where puzzle='"+puzzle+"' and run='"+run+"' group by n").fetchall()
    means={ x[0] : np.median([ float(y) for y in x[1].split(",")]) for x in base }

    for name in names:
        data=con.execute("select n,currTime from rows where puzzle='"+puzzle+"' and name='"+name+"' and run='"+run+"'  order by n").fetchall();
        style="-"# if selName==name else "--"
        lineWidth=2.0 if selName==name else 1.0
        marker="o" if selName==name else " "
        zorder=2 if selName==name else 0
        alpha=1.0 if selName==name else 0.5
        try:
            xx=[x[0] for x in data]
            yy=[means[x[0]]/x[1] for x in data]
            ax.loglog(xx , yy, style, label=name, linewidth=lineWidth, marker=marker, alpha=alpha)
        except:
            pass

    ax.set_ylabel('(Time for median) / (Time for selected)')
    ax.set_xlabel('Problem size (n)')

    plt.title('puzzle={0}, user={1}, run={2}'.format(puzzle,selName,run))
    #plt.show()

def plot_speedup_for_puzzle_vs_best(puzzle,selName,run):
    plt.clf()
    plt.cla()
    ax=plt.gca()

    base=con.execute("select n,min(currTime) from rows where puzzle='"+puzzle+"' and run='"+run+"' group by n").fetchall()
    mins={ x[0] : x[1] for x in base }

    for name in names:
        data=con.execute("select n,currTime from rows where puzzle='"+puzzle+"' and name='"+name+"' and run='"+run+"'  order by n").fetchall();
        style="-"# if selName==name else "--"
        lineWidth=2.0 if selName==name else 1.0
        marker="o" if selName==name else " "
        zorder=2 if selName==name else 0
        alpha=1.0 if selName==name else 0.5
        try:
            ax.loglog( [x[0] for x in data], [mins[x[0]]/x[1] for x in data], style, label=name, linewidth=lineWidth, marker=marker, alpha=alpha)
        except:
            pass
               

    ax.set_ylabel('(Time for best) / (Time for selected)')
    ax.set_xlabel('Problem size (n)')

    ymin,ymax=ax.get_ylim()
    ax.set_ylim(ymin,2.0)

    plt.title('puzzle={0}, user={1}, run={2}'.format(puzzle,selName,run))
    #plt.show()

#plot_speedup_for_puzzle_vs_best("median_bits", "ajw10_ml3411")
#plt.show()

#def calc_puzzle_ranks(puzzle, name):
#    v=con.execute("select name,count(n),sum(currTime) from rows where puzzle='"+puzzle+"' and checkStatus=0 group by name").fetchall()




def make_all_inserts(run):
    for name in opt_in_names:
        for puzzle in puzzles:
            sys.stderr.write("  {} {}\n".format(name,puzzle))
            
            plot_perf_for_puzzle(puzzle, name, run)
            plt.gcf().set_size_inches(12.0,8.0)
            
            dir='graphs/{1}/{2}'.format(puzzle,name,run)
            print(dir)
            try:
                os.makedirs(dir)
            except:
                pass
            plt.savefig('graphs/{1}/{2}/{1}_{0}_{2}_perf.pdf'.format(puzzle,name, run))

            #plot_speedup_for_puzzle_vs_median(puzzle, name,run)
            #plt.gcf().set_size_inches(12.0,8.0)
            #plt.savefig('out/{1}_{0}_speedup_over_median.pdf'.format(puzzle,name,run))

            #plot_speedup_for_puzzle_vs_best(puzzle, name,run)
            #plt.gcf().set_size_inches(12.0,8.0)
            #plt.savefig('out/{1}_{0}_speedup_vs_best.pdf'.format(puzzle,name,run))

toRun=sys.argv[1]

make_all_inserts(toRun)
