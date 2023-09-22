#include <iostream>
#include <string>
#include <iomanip>
#include <fstream>
#include<list>
using namespace std;



//les <img src="./smiley.jpg" alt="Smiley smiling!" width="200" height="200"> should be replace by our own images



void searchinFile(string& text, const string& word1, const string& replaceWord1, const string& word2, const string& replaceWord2) {
    string copie = text;
    int textLength = copie.length();
    int maxLength = max(word1.length(), word2.length());
    int word1length = word1.length();
    int word2length = word2.length();
    for (int i = 0; i <= textLength; i++) { //condition on i in order to go above the number of words in the text
        bool found1 = true;
        for (int j = 0; j < word1length; j++) {
            if ((i+j)>textLength) {
                break;
            } else {
                if (copie[i + j] != word1[j]) {
                    found1 = false;
                    break;
                } 
            }
        }
        bool found2 = true;
        for (int k = 0 ; k < word2length ; k++) {
            if ((i+k)>textLength) {
                break;
            } else {
                if (copie[i + k] != word2[k]) {
                    found2 = false;
                    break;
                } 
            }
        }   
        if (found1) {
            //Replace a word : https://cplusplus.com/reference/string/string/replace/
            if (copie[i-1]=='/') {
                cout << "A faire" << endl;
                string lien="http://zebroid.ida.liu.se/fakenews/trolly.jpg";
                lien_Lenght=lien.length()
                //replace with our link to the photo of trolly
                copie.replace(i-2, lien_length, lien);//pour remplacer aussi le ./
                i=i+12;//car ./smiley.jpg est 12 caracteres
            } else {
                copie.replace(i, word1length, replaceWord1);
                found1=true;
            }
        }
        if (found2) {//enlever memory si tout d'un coup
            if (copie[i-1]=='/'){
                break; //Don't replace if the word is the name of an image
            } else {
                copie.replace(i, word2length, replaceWord2); 
                found2=true;
            }
                
        }
    }
    response.content_length=copie.length();
    text=copie;
}

int main() {
    string Text;
    Text.assign(response.body, response.body+response.content_length);
    //https://www.geeksforgeeks.org/convert-char-to-string-in-cpp/

    /*
    string sWord1 {"Smiley"};
    string replaceWord1 {"Trolly"};
    string sWord2 {"Stockholm"};
    string replaceWord2 {"Linköping"};
    
    cout << "Enter the text : "; //To test. We will have the segment
    getline(cin, Text);
    
    searchinFile(Text, sWord1, replaceWord1, sWord2, replaceWord2);
    */

    searchinFile(response.body, "Smiley", "Trolly", "Stockholm", "Linköping");
    cout << Text << endl;
    return 0;
}
