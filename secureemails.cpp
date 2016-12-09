#include <iostream>
#include <typeinfo>
#include <string>
#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>
/* my thoughts on the tables atm. haven't entered them yet.
g++ secureemails.cpp -lsqlite3
create table USERS( USERNAME text primary key, PASSWORD text, MESSAGECOUNT integer, CONTACTLIST text);

I'm thinking a username should be unique enough
but we might want to add an ID somewhere in there

create table "MESSAGE(RECIPIENT text foreign key, SENDER text foreign key, 
PASSPHRASE text, MESSAGE text);" 

check this shit out for reference:
https://www.tutorialspoint.com/sqlite/sqlite_c_cpp.htm
*/

bool user = false;

void encrypt(std::string* s) {
    std::string temp = *s;
    int index = 0;
    int t;
    for(int i = 0; i < s->length(); i+=2) {
        t = (*s)[i] + 13;
        if(t > 126) {
            if(t-127 > 6) t = 32 + t-127;
            else t = 32 + t-127;
        }
        (*s)[i] = (char)t;
        temp[index] = (*s)[i];
        index++;
    }
    for(int i = 1; i < s->length(); i+=2) {
        t = (*s)[i] + 13;
        if(t > 126) {
            if(t-127 > 6) t = 32 + t-127;
            else t = 32 + t-127;
        }
        (*s)[i] = (char)t;
        temp[index] = (*s)[i];
        index++;
    }
    *s = temp;
}

void decrypt(std::string* s) {
    std::string temp = *s;
    int t;
    int index = 0;
    for(int i = 0; i < s->length(); i+=2) {
        temp[i] = (*s)[index];
        index++;
    }
    for(int i = 1; i < s->length(); i+=2) {
        temp[i] = (*s)[index];
        index++;
    }
    *s = temp;
    for(int i = 0; i < s->length(); i++) {
        t = (*s)[i];
        if(t > 39 && (t - 14) < 32) {
            int o = 14-(t-32);
            (*s)[i] = (char)(127-o);
        } else if((t - 13) < 32) {
            int o = 13-(t-32);
            (*s)[i] = (char)(127-o);
        }
        else {
            (*s)[i] -= 13;
        }
    }
}

/**
 * 
 * */
static int callback(void *NotUsed, int argc, char **argv, char **azColName){
    user = ((int)(argv[0][0]-'0') > 0);
    int i;
    for(i=0; i<argc; i++){
        std::string s = argv[i] ? argv[i] : "";
        decrypt(&s);
        //printf("%s = %s\n", azColName[i], s.c_str());
    }
    printf("\n");
    return 0;
}
/**
 * counts the users messages in the database given a certain user
*/
int countMessages(std::string* name, sqlite3* db){
    //make query to count unique messages
    char *zErrMsg = 0;
    int rc;
    const char* data = "Callback function called";
    /* Create SQL statement. this should count the messages if my syntax/logic are right */ 
    std::cout << "Here is the amount of messages you have:" << std::endl;
    std::string sql = ("SELECT COUNT(*) from MESSAGES where RECIPIENT = '" + *name +"';").c_str();
    
    /* Execute SQL statement */
    rc = sqlite3_exec(db, sql.c_str(), callback, (void*)data, &zErrMsg);
    
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }else{//might want to delete this to avoid spam
        fprintf(stdout, "Messages counted successfully\n");
    }
}

/**
 * prints the contacts of given user in the database
 * */
void printContacts(std::string* name, sqlite3* db){
    char *zErrMsg = 0;
    int rc;

    const char* data = "Callback function called";
    //make query to print contacts (select all from contact list field)
    /* Create SQL statement. this should work if my syntax/logic are right */ 
    std::cout << "Here is a list of your contacts:" << std::endl;
    std::string sql = ("SELECT SENDER from MESSAGES where RECIPIENT = '" + *name +"';").c_str();//maybe * before sender
    
    /* Execute SQL statement */
    rc = sqlite3_exec(db, sql.c_str(), callback, (void*)data, &zErrMsg);
    
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }else if(rc == 1){//might want to delete this to avoid spam
        fprintf(stdout, "Contacts printed successfully.\n");
    }
    else{
        fprintf(stdout, "You have no contacts:[\n");
    }
}



/** 
 * goes through the logic to sign in to an account given from main or 
 * possibly after registering for an account
 * uses the information given to verify validity of username and password
 * */
void signIn(std::string* name, std::string* password, sqlite3* db){
    bool loop = true;
    bool success = true;
    int rc;
    std::string retryname, retrypassword;
    const char* data = "Callback function called";
    char *zErrMsg = 0;
    int i = 0;
    while(loop){
    if(i >0){
        std::cout << "Retry another screenname: ";
        std::cin >> retryname;
        *name = retryname;
        std::cout << "Retry your password: ";
        std::cin >> retrypassword;
        *password = retrypassword;
    }
    //do logic to check if name and password are in db
    //if in db change success to true 
    // Create SQL statement 
    encrypt(name);
    encrypt(password);
    
    std::string sql = ("SELECT COUNT(*) from USER where PASSWORD = '" + *password + "' AND USERNAME = '" + *name + "';").c_str();
    
    // Execute SQL statement 
    rc = sqlite3_exec(db, sql.c_str(), callback, (void*)data, &zErrMsg);
    //check validity
    
        if( rc != SQLITE_OK ){//TODO: perhaps check NULL
            //query was not successful (BAD, means nothing account doesn't exist)
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        success = user;
        
        if(success){
            std::cout << "Signing in to your account:" << std::endl;
            countMessages(name, db);
            printContacts(name, db);
            loop = false;
        } else{
            std::cout << "That username and password combination is not valid. Try again." << std::endl;
        }
    i++;
    }
    
}

/**
 * Goes through the logic to register an account
 * inputs the username and password entered in main to do so, 
 * creates an instance of the account in db when valid
 * input is entered
 * */
void registerAccount(std::string* name, std::string* password, sqlite3* db){
    bool loop = true;
    bool success = false;
    std::string retryname, retrypassword;
    int response, newResponse;
    char *zErrMsg = 0;
    int rc = sqlite3_open("mail.db", &db);
    const char *data = "Callback function called";
    //rot53(name);
    //rot53(password);
    int i = 0;
    //prompt for username and password
    while(loop){
        if(i >0){
            std::cout << "Retry another screenname: ";
            std::cin >> retryname;
            *name = retryname;
            std::cout << "Retry your password: ";
            std::cin >> retrypassword;
            *password = retrypassword;
        }
    //do logic to check if name and password are in db
    
    // Create SQL statement 
    encrypt(password);
    encrypt(name);
    
    std::string sql = ("SELECT COUNT(*) from USER where PASSWORD = '" + *password + "' AND USERNAME = '" + *name + "';").c_str();
    // Execute SQL statement 
    rc = sqlite3_exec(db, sql.c_str(), callback, (void*)data, &zErrMsg);
  
    //check validity
        if( rc != SQLITE_OK ){//TODO: perhaps check NULL
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        else if(user){
            //query was successful
            std::cout << "That username and password combination is already taken :[" << std::endl;
        }
        else{
            success = true;
        }
        
        if(success){
            const char* data = "callback function called.";
            /* Create SQL statement */
            //if NOT in db (see if above) change success to true and create instance of name in db
            
            std::string sql2 = ("INSERT INTO USER (USERNAME, PASSWORD, MESSAGECOUNT, CONTACTLIST) VALUES ('" + *name + "', '" + *password + "', NULL, NULL); ").c_str();
            
            /* Execute SQL statement */
            rc = sqlite3_exec(db, sql2.c_str(), callback, (void*)data, &zErrMsg);
            if( rc != SQLITE_OK ){
                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                sqlite3_free(zErrMsg);
            }else{
                fprintf(stdout, "Account created successfully\n");
            }     
            //std::cout << "Account registration was successful." << std::endl;
            loop = false;
        }
    i++;
    }
    if(success){
        std::cout << "Would you like to sign in? (1 if yes, 0 if no): ";
        std::cin >> response;
        if(std::cin.fail()) response = 3;
        
        if(response ==1){ 
            signIn(name,password, db); 
        }
        else if(response == 0){}
        else{
        while(response != 1 || response != 0 || std::cin.fail()){
            std::cout << "Your input is invalid. Please enter 1 or 0: ";
            std::cin.clear();
            std::cin.ignore();
            std::cin >> newResponse;
            response = newResponse;
            if(response ==1){ 
                signIn(name, password, db);
                break;
            }
            else if(response == 0){};
                break;
            }
        }
    }
    sqlite3_close(db);
    user = false;
}

void print_all(const char* database) {
    sqlite3 *db;
    char *zErrMsg = 0;
    const char* data = "callback function called.";
    int rc = sqlite3_open(database, &db);
    std::string sql = "SELECT * FROM USER ";
    rc = sqlite3_exec(db, sql.c_str(), callback, (void*)data, &zErrMsg);
    sqlite3_close(db);
}

main(){
    sqlite3* db;
    int rc;
    char *zErrMsg = 0;
    const char* data = "Callback function called";
    //begin constructing database
    rc = sqlite3_open("mail.db", &db);
    if( rc ){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1; 
    };
    
    /*
    //logic used to construct db
    //insert create table in string with semicolon at the end
    std::string sql = "create table USER( USERNAME text PRIMARY KEY NOT NULL, PASSWORD text NOT NULL, MESSAGECOUNT integer, CONTACTLIST text);";
    rc = sqlite3_exec(db, sql.c_str(), callback, (void*)data, &zErrMsg);
    
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }else{
        fprintf(stdout, "Table created successfully\n");
    }    
    //insert create table in string with semicolon inside when we figure out how we're gonna do the tables
    std::string sql2 = "create table MESSAGES( MESSAGEID integer auto increment,RECIPIENT text NOT NULL, SENDER text NOT NULL, PASSPHRASE text, MESSAGE text, FOREIGN KEY(RECIPIENT) REFERENCES USER(USERNAME), FOREIGN KEY(SENDER) REFERENCES USER(USERNAME) );";
    
    rc = sqlite3_exec(db, sql2.c_str(), callback, (void*)data, &zErrMsg);
    
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }else{
        fprintf(stdout, "Table created successfully\n");
    }    
    //end of constructing database
    */
    int decision, newDecision;
    std::string name,password;
    std::cout << "Enter (1) to sign in or enter (0) to register a new account" << std::endl;
    std::cin >> decision;
    if(std::cin.fail()){
        decision = 3;
    }
    //do whatever they choose
    if(decision == 1){ 
            std::cout << "Yay! let's sign in to your account." << std::endl;
        
            std::cout << "enter your screenname: ";
            std::cin >> name;
            std::cout << "enter your password: ";
            std::cin >> password;
        
            signIn(&name, &password, db);
    } else if (decision == 0){ 
            std::cout << "enter your screenname: ";
            std::cin >> name;
            std::cout << "enter your password: ";
            std::cin >> password;
        
            std::cout << "Registering account now..." << std::endl;
            registerAccount(&name, &password, db); 
    }
    else{
        //otherwise loop throough until correct thing is entered
        while(decision != 1 || decision != 0 || std::cin.fail()){
            std::cin.clear();
            std::cin.ignore();
            std::cout << "Your input is invalid. Please enter 1 or 0:" << std::endl;
            std::cin >> newDecision;
            decision = newDecision;
            if(decision == 1){ 
                std::cout << "let's sign in to your account. Please enter your information:" << std::endl;
        
                std::cout << "enter your screenname: ";
                std::cin >> name;
                std::cout << "enter your password: ";
                std::cin >> password;
        
                signIn(&name, &password, db);
                break;
            } else if (decision == 0){ 
            std::cout << "enter your screenname: ";
            std::cin >> name;
            std::cout << "enter your password: ";
            std::cin >> password;
        
            std::cout << "Yay! let's register an account for you m8" << std::endl;
            registerAccount(&name, &password, db); 
            break;
            }
        }
    }
    
    print_all("mail.db");
    
    sqlite3_close(db);
    return 0;
}
