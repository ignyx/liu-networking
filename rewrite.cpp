#include <iostream>
#include <string>
#include <iomanip>
#include <fstream>
using namespace std;

//See how to take file and carry them segment by segment


"""
Pour l'instant : juste recherche et remplacement de mot
"""


// Function to search for a word octet by octet
bool searchWord(string& text, const string& word, const string& replaceWord) {
    int textLength = text.length();
    int wordLength = word.length();

    for (int i = 0; i <= textLength - wordLength; i++) { //condition on i in order to not depasser le nbr de mot du texte
        bool found = true;
        for (int j = 0; j < wordLength; j++) {
            if (text[i + j] != word[j]) {
                found = false;
                break;
                //If we are not false but there are no letter left : create a memory (or in out) and check with the other segment
                //--> reprendre la ou on en est meme avec un autre segment
            }
        }
        if (found) {
            text.replace(i, wordLength, replaceWord); // see Notion for link : .replace (position, taille, mot qui remplace)
            return true;
            //Need to check if we can replace it (not stokholm in an image name for example) --> Look at the character around ?
            //Need to change the content length : If Linköping because of the ö. How to access it ?
        }
    }

    return false;
}

int main() {
    string Text;
    string sWord {"Smiley"};
    string replaceWord {"Trolly"};

    cout << "Enter the text : "; //Pour tester, on aura le segment
    getline(cin, Text);


    if (searchWord(Text, sWord, replaceWord)) { //Pour tester, on renverra au client
        cout << "Word found in the text." << endl;
    } else {
        cout << "Word not found in the text." << endl;
    }
    cout << Text << endl;
    return 0;
}




//treat them by octet
//one octet is one letter
//if the first letter correspond to the first of one our words to change : stock it.
//Otherwise : send it to the client.


//See how to send them back