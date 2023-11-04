#include <iostream>
#include <string>
#include <iomanip>
#include <fstream>
#include<list>
using namespace std;



void searchinFile(string& text, const string& word1, const string& replaceWord1, const string& word2, const string& replaceWord2) {
    string copie = text;
    int textLength = copie.length();
    int word1length = word1.length();
    int word2length = word2.length();
    for (int i = 0; i <= textLength; i++) { 
        bool found1 = true;
        for (int j = 0; j < word1length; j++) {
            if ((i+j)>textLength) {
                break;//condition on i+j in order to not go above the number of words in the text
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
                break; //condition on i+k in order to not go above the number of words in the text
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
                string lien="http://zebroid.ida.liu.se/fakenews/trolly.jpg";
                lien_Lenght=lien.length()
                //replace with our link to the photo of trolly
                copie.replace(i-2, lien_length, lien);//pour remplacer aussi le ./
                i=i+12;//car ./smiley.jpg est 12 caracteres

//ca fait des problèmes
//le suivant est modifé mais pas après


            } else {
                copie.replace(i, word1length, replaceWord1);
                found1=true;
                i=i+6;
            }
        }
        if (found2) {
            if (copie[i-1]=='/'){
                break; //Don't replace if the word is the name of an image
            } else {
                copie.replace(i, word2length, replaceWord2); 
                found2=true;
                i=i+9;
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

    searchinFile(Text, "Smiley", "Trolly", "Stockholm", "Linköping");
    return 0;
}
