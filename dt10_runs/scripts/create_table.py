#!/bin/python

import sys
import os
import math
import re

import sqlite3

con = sqlite3.connect("all.db")
cur = con.cursor()
cur.execute("drop table if exists rows")
cur.execute("create table rows (name STRING, run STRING, opt_in INTEGER, puzzle STRING, n INTEGER, currTime REAL, totalTime REAL)")

data = open("all.rows")
tuples = [tuple([y.strip() for y in x.split(",")]) for x in data];

cur.executemany("insert into rows values (?,?,?,?,?,?,?)", tuples);
con.commit()
