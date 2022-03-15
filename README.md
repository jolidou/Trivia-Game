# Trivia Game - Joli Dou

---

# Overview

## Demo Video:

[demonstration video](https://youtu.be/piVNRdXTtpg)

## Implementing Specifications:

1. Getting Trivia from API:
   In the file questions_request.py, I used the requests library and a custom URL from the Trivia API site specifying the type and number of questions to generate. I chose to retrieve 1 True/False question each time from the animal category, so the query arguments in the URL were amount = 1, category = 27, and type = boolean. Then, I used Postman to check that a GET request to this URL returned the desired response, and I used the requests library function requests.request to retriev the response within my code. Then, I uploaded this to the 6.08 server in the trivia folder.

2. Long-term Statistics:
   During each game, until the user stops the game there are local variables for score, correct (the number of correct answers in that game), and incorrect (the number of incorrect answers in that game). When the user stops the game, I pass these variables in as parameters for the GET request. On the server side, in my get_database.py file, my request_handler function takes in the request, checks that it is a GET request, finds the relevant key for each field, and gets the value of each, casting it to the appropriate data type (score, correct, and incorrect are all ints).

   Then, I call insert_into_database, passing in the score and number of correct and incorrect answers from the request. Within insert_into_database, I attempt to get the data of the user from the request using SELECT queries and a WHERE user = request user condition. If the data does not exist, I perform an INSERT operation with the user, score, and correct/incorrect from the request. If the queries do return existing user information, I add the local score, number correct, and number incorrect from the request to the returned information, then store it again at that user using the UPDATE operation.

3. Scoreboard:
   When a user stops their game and thus sends a GET request submitting their game data to the database, the query has a scoreboard parameter that is either "True", if the user performs a long press on the ResetButton, or "False" for a short press. If the parameter is "True," my request handler function returns the lookup all function, which connects to the trivia database and executes a SELECT query to select all fields with the \* operator. Then, I sort the returned rows, where each row has a user's information, using the Python sort function on the value at index 1 of each row with reverse = True, which means sorting users by highest to lowest score. Finally, I build the HTML list scoreboard by creating a string with the ordered list tag (ol) and adding each user row as a list item (li), with the HTMl tag formatting within the string.

4. Repeatable:
   The game repeats when the user decides to end the current game and start a new one. Rather than having to reset the whole ESP32, the user can simply press the ResetButton, an instance of the Button class from previous labs initialized on pin 34. When the game resets, the current game data (user, scoreboard, score, number correct, and number incorrect) is sent to the SQL database with an HTTP GET request as explained previously, and all equivalent local values (score, correct, incorrect) are reset to 0. For the ResetButton, there is a state machine within the Button class, where for presses longer than the debounce duration, there is a short press (which returns flag 1) and long press state (which returns flag 2), and in the broader code, I use a reset_state variable to store the current flag from each ResetButton update and an old_reset_state variable, so the code runs only when there is some change to the ResetButton state.
5. Game Implementation:
   When the ESP32 starts up, the user must press button 45 to start the game, and this performs a GET request for a question. I chose to have my questions in the category "Animal" (27 on the API documentation) and be True/False. The user must then answer "True" (by pressing the button on pin 39) or "False" (by pressing the button on pin 38), and if this answer matches the answer from the API, then they answered correctly, and their local score increases by 1 point. If incorrect, their local score decreases by 1 point (negatives allowed so the game does not end before the user wants it to). Upon answering, the LCD also updates to show whether the answer was correct or incorrect, the current score, and what the user answered.

   The user continues the game by pressing the button on pin 45 to get a new question, and this process of answering, getting questions, and updating the score continues until the user decides to stop the game, by pressing the button on pin 34. If the user long-presses this button, the scoreboard query argument is "True", and the scoreboard will be returned as one can see in the browser. If the button is short-pressed, the scoreboard query argument is "False," and the game data will only be added to the database.

## Design Strategies:

1. Game Design
   I chose True/False questions to more easily restrict the space of allowed values for user input, for safety purposes. I also chose to use animal category questions out of personal interest, but this can be changed by adjusting the category parameter in the API URL of the questions_request.py file.
2. Scoreboard
   I created a helper function in my get_database.py file to build up a string containing HTMl tags and formatting. In this function, lookup_all, I perform a SQL query selecting all fields (SELECT \*) and fetch all values from the database. Then, I sorted the outputs from highest to lowest score. For formatting, I decided to use an ordered list so my scoreboard would appear like a ranking, and to do this, I used "ol" tags for the overall list and "li" tags for each list item (each user and their data).
3. Display and States
   Upon starting up the ESP32, I display a welcome message so the user knows to press button 45 to start. Upon each (user-set) repetition of the game after that point, the message instead says that the game has been reset, and to press button 45 to continue. I also chose to display, after each user answer, their score, correct/incorrect, and what the user had answered, as I thought this would be helpful for players and help them check that they were playing the game as they intended to.

   Each button used function using a state machine, as defined within the Button class, which used a transitional state to account for button debouncing. Furthermore, the buttons had current and previous states (reset_state, old_reset_state, etc.) so only when a button had updated would the code block run. Also, I used mode 0 (as defined with the mode variable) to be the getting question state, and after the question button (on pin 45) was pressed, I transitioned to mode 1, the answering state, so the system would need to take in a user input for True or False before transitioning back to mode 0 (where the user can get a new question or end the game and start a new one).

## Resources

1. Request Handler (for adding game data to the database)

```py
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

```

2. Creating a Scoreboard

```py
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
```

3. Game Repeatability

```cpp
  reset_state = ResetButton.update();
  if (reset_state != 0 && reset_state != old_reset_state) {
    //send score to server
    if (reset_state == 1) {
      sprintf(scoreboard, "False");
    } else if (reset_state == 2) {
      sprintf(scoreboard, "True");
    }

    int offset = 0;
    // Make a HTTP request:
    Serial.println("SENDING GAME DATA");
    //get location on campus
    request[0] = '\0'; //set 0th byte to null
    offset = 0; //reset offset variable for sprintf-ing
    offset += sprintf(request + offset, "GET http://608dev-2.net/sandbox/sc/jolidou/trivia/get_database.py?user=%s&score=%d&scoreboard=%s&correct=%d&incorrect=%d HTTP/1.1\r\n", user, score, scoreboard, correct, incorrect);
    offset += sprintf(request + offset, "Host: 608dev-2.net\r\n");
    offset += sprintf(request + offset, "Content-Type: application/x-www-form=urlencoded\r\n");
    offset += sprintf(request + offset, "\r\n");
    do_http_request(server, request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);

    Serial.flush();
    Serial.println(response);
    score = 0;
    correct = 0;
    incorrect = 0;

    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0, 1);
    sprintf(output, "Game reset: press button 45 to start new game!        ");
    tft.println(output);
  }
  old_reset_state = reset_state;
```

4. False Answer Case (Analogous to True)

```cpp
    if (FalseButton.flag != 0) {
    Serial.flush();
    sprintf(answer, "False");
    if (strncmp(answer,expected_answer,strlen(expected_answer))) {
        memset(output, 0, sizeof(output));
        correct++;
        score = correct - incorrect;
        sprintf(output, "Correct!     \nYou answered:\"%s\"      \nScore:%d            ", answer, score);
        tft.fillScreen(TFT_BLACK);
    } else {
        memset(output, 0, sizeof(output));
        incorrect++;
        score = correct - incorrect;
        sprintf(output, "Wrong!       \nYou answered:\"%s\"      \nScore:%d             ", answer, score);
        tft.fillScreen(TFT_BLACK);
    }
    mode = 0;
    }

```

---

# Summary

In this project, I implemented a "Trivia Game" with a server script to get questions and their answers from a Trivia API, and server scripts to store a SQLite database of past game data. For each unique user, I either inserted their local game data (user, score, number correct answers, number incorrect answers) or updated their existing data from previous games by adding their local game data. For each game, users could answer True/False animal questions, with +1 point for each correct answer and -1 point for each incorrect answer. Their game data is submitted when they choose to reset the game using the ResetButton. Some key concepts were (1)server scripts and communication, (2)SQL queries, (3)parsing (JSON and strings), and (4)constructing HTML within a string. Hardware concepts included buttons and the LCD display.
