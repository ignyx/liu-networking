#include <iostream>
#include <string>
#include <iomanip>
#include <fstream>
using namespace std;

//See how to take file and carry them segment by segment


//separer en fct : searchWord, replaceWord


//les <img src="./smiley.jpg" alt="Smiley smiling!" width="200" height="200"> should be replace by our own images


// Function to search for a word octet by octet


//return a partie memoire ?

void searchinFile(string& text, const string& word1, const string& replaceWord1, const string& word2, const string& replaceWord2) {
    int textLength = text.length();
    int maxLength = max(word1.length(), word2.length());
    int word1length = word1.length();
    int word2length = word2.length();
    for (int i = 0; i <= textLength - maxLength; i++) { //condition on i in order to go above the number of words in the text
        bool found1 = true;

        for (int j = 0; j < word1length; j++) {
            if (text[i + j] != word1[j]) {
                found1 = false;
                break;
                //If we are not false but there are no letter left : create a memory (or in out) and check with the other segment
                //--> Continue at the same point but with the other segment
            }
        }
        bool found2 = true;
        for (int k = 0 ; k < word2length ; k++) {
            if (text[i + k] != word2[k]) {
                found2 = false;
                break;
                //If we are not false but there are no letter left : create a memory (or in out) and check with the other segment
                //--> Continue at the same point but with the other segment
            }
        }
        if (found1) {
            //Replace a word : https://cplusplus.com/reference/string/string/replace/
            if (text[i-1]=='/') {
                cout << "A faire" << endl;
                //Change Content-Length Header (our length)
                //replace with our link to the photo of trolly
            } else {
                text.replace(i, word1length, replaceWord1);
                found1=true;
            }
        }
        if (found2) {
            if (text[i-1]=='/'){
                break; //Don't replace if the word is the name of an image
            } else {
                text.replace(i, word2length, replaceWord2); 
                //Change Content-Length Header (+1)
                found2=true;
            }
                
        }
    }
}

int main() {
    string Text;
    string sWord1 {"Smiley"};
    string replaceWord1 {"Trolly"};
    string sWord2 {"Stockholm"};
    string replaceWord2 {"Linköping"};

    cout << "Enter the text : "; //To test. We will have the segment
    getline(cin, Text);

    searchinFile(Text, sWord1, replaceWord1, sWord2, replaceWord2);
    cout << Text << endl;
    return 0;
}




//treat them by octet
//one octet is one letter
//if the first letter correspond to the first of one our words to change : stock it.
//Otherwise : send it to the client.


//See how to send them back
