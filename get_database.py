#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sun Mar 13 00:04:37 2022

@author: jolidou
"""

import sqlite3
import random
import string

trivia_db = "/var/jail/home/jolidou/trivia/trivia.db" # just come up with name of database

def create_database():
    conn = sqlite3.connect(trivia_db)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # move cursor into database (allows us to execute commands)
    c.execute('''CREATE TABLE IF NOT EXISTS scoreboard_table (user text, current_score int, num_correct int, num_incorrect int);''') # run a CREATE TABLE command
    conn.commit() # commit commands (VERY IMPORTANT!!)
    conn.close() # close connection to database

create_database()  #call the function!

def insert_into_database(user, score, correct, incorrect):
    conn = sqlite3.connect(trivia_db)
    c = conn.cursor()
    curr_score = get_score(user)
    curr_correct = get_correct(user)
    curr_incorrect = get_incorrect(user)
    if (curr_score == None or curr_correct == None or curr_incorrect == None):
        c.execute('''INSERT into scoreboard_table (user, current_score, num_correct, num_incorrect) VALUES (?, ?, ?, ?);''', (user, score, correct, incorrect))
    else:
        updatedScore = score + curr_score
        updatedCorrect = correct + curr_correct
        updatedIncorrect = incorrect + curr_incorrect
        c.execute('''UPDATE scoreboard_table SET current_score = ?, num_correct = ?, num_incorrect = ? WHERE user = ?;''', (updatedScore, updatedCorrect, updatedIncorrect, user))
    conn.commit()
    conn.close()

def get_score(user):
    conn = sqlite3.connect(trivia_db)
    c = conn.cursor()
    score = c.execute('''SELECT current_score FROM scoreboard_table WHERE user = ?;''', (user,)).fetchone()
    conn.commit()
    conn.close()
    if (score == None):
        return score
    else:
        return score[0]

def get_correct(user):
    conn = sqlite3.connect(trivia_db)
    c = conn.cursor()
    correct = c.execute('''SELECT num_correct FROM scoreboard_table WHERE user = ?;''', (user,)).fetchone()
    conn.commit()
    conn.close()
    if (correct == None):
        return correct
    else:
        return correct[0]

def get_incorrect(user):
    conn = sqlite3.connect(trivia_db)
    c = conn.cursor()
    incorrect = c.execute('''SELECT num_incorrect FROM scoreboard_table WHERE user = ?;''', (user,)).fetchone()
    conn.commit()
    conn.close()
    if (incorrect == None):
        return incorrect
    else:
        return incorrect[0]
    
def lookup_all():
    conn = sqlite3.connect(trivia_db)
    c = conn.cursor()
    things = c.execute('''SELECT * FROM scoreboard_table;''').fetchall()
    things.sort(key = lambda x: int(x[1]), reverse = True) 
    out = ""
    out += "<b>Trivia Scoreboard</b>"
    out += "<ol>"
    for u in things:
        out+= ("<li>" + str(u)+ "</li>")
    out += "</ol>"
    conn.commit()
    conn.close()
    return out


def request_handler(request):
    if request["method"] == "GET":
        user = ""
        score = 0
        correct = 0
        incorrect = 0
        try:
            user = str(request['values']['user'])
            score = int(request['values']['score'])
            correct = int(request['values']['correct'])
            incorrect = int(request['values']['incorrect'])
            insert_into_database(user, score, correct, incorrect)
            if (str(request['values']['scoreboard']) == 'True'):
                return lookup_all()
            else:
                return ("Added new score to scoreboard!")
        except Exception:
            return "Error: missing fields or wrong format"
    
    else:
        return 'Unsupported HTTP operation!'