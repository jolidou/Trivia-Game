#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sun Mar 13 00:05:03 2022

@author: jolidou
"""

import requests

url = "https://opentdb.com/api.php?amount=1&category=27&type=boolean"

payload={}
headers = {
  'Cookie': 'PHPSESSID=9b29f56a96cb2b1ea6f3b957e874e7ed'
}

response = requests.request("GET", url, headers=headers, data=payload)

def request_handler(request):
    return str(response.text)

#GET: log in to database (cursor) post database
#GET trivia questions: 