/*Maverick Kieu
 * kieu92
 * 115028852
 * CMSC131-0402
 * I pledge on my honor that I have not given or received any 
 * unauthorized assistance on this assignment/examination.
 */

/**

 * CMSC131-010x,030x,040x Project 3 Hangman
 * 
 * You must complete "ALL" methods and also make your program work as an 
 * stand-alone Java program.
 * 
 * Submit to the submit server and check your grade. 
 * 
 * String library:
 * https://docs.oracle.com/javase/8/docs/api/java/lang/String.html
 * 
 * @author CMSC131 instructors
 *
 */

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.Random;
import java.util.Scanner;

public class Hangman {

    final static int nWords = 2266;

    /**
     * Do NOT modify this method This method chooses a word from a text file
     * randomly.
     * 
     * @param f
     * @return
     * @throws FileNotFoundException
     */
    public static String chooseWord(File f, Random r) throws FileNotFoundException {
        Scanner fileIn = new Scanner(f);
        try {
            int randInt = r.nextInt(nWords);
            int i = 0;
            while (i != randInt) {
                fileIn.nextLine();
                i++;
            }
            return fileIn.nextLine();
        } catch (Exception e) {
            return "";
        } finally {
            fileIn.close();
        }
    }

    /**
     * This method returns a string that contains underscore characters only. The
     * number of underscore characters must be same to the number of word passed as
     * the input parameter.
     * 
     * @param word
     *            the word selected from the chooseWord method
     * @return
     */
    public static String createDisguisedWord(String word) {
        String disguise = "";
        for(int i = 0; i<word.length(); i++) {
        	disguise = disguise+"_";
        }
        return disguise;
    }

    /**
     * This method returns true if the guess entered by the player occur in the
     * secret word. Otherwise it return false.
     * 
     * @param guess
     *            a string that contains one or more characters
     * @param secretWord
     *            the word selected from the chooseWord method
     * @return
     */
    public static boolean isValidGuess(String guess, String secretWord) {
    	boolean result = true;
        if(!secretWord.contains(guess)) result = false;
        return result;
    }

    /**
     * This method finds all occurrences of guess in secretWord and returns new
     * disguisedWord with letters revealed after guess
     * 
     * @param guess
     * @param secretWord
     * @param disguisedWord
     * @return
     */
    public static String makeGuess(String guess, String secretWord, String disguisedWord) {
    	int g = guess.length();
    	if(secretWord.contains(guess)) {
    		for(int i=0; i<secretWord.length()-g+1; i++) {
    			if(guess.equals(secretWord.substring(i,i+g))) {
    				disguisedWord = disguisedWord.substring(0, i)+guess+disguisedWord.substring(i+g);
    			}
    		}
    	}
        return disguisedWord;
    }

    /**
     * This method returns true if the (partially) revealed word (disguisedWord) is
     * identical to the secret word. Otherwise it returns false. If all letters in
     * the secret word are revealed, those two should be identical.
     * 
     * @param secretWord
     * @param disguisedWord
     * @return
     */
    public static boolean isWordRevealed(String secretWord, String disguisedWord) {
        boolean result = true;
       for(int i = 0; i<secretWord.length(); i++) {
        	if(secretWord.charAt(i) != disguisedWord.charAt(i)) result = false;
        }
        return result;
    }

    /**
     * This method computes and returns an integer of which value is 3 greater than
     * the number of unique letters in the secret word.
     * 
     * @param word
     *            the word selected from the chooseWord method
     * @return
     */
    public static int initGuesses(String word) {
        int num = 3;
        for(int i=0; i<word.length(); i++) {
        	boolean repeat = false;
        	for(int j=0; j<i; j++) {
        		if(word.charAt(i)==word.charAt(j)) {
        			repeat = true;
        			break;
        		}
        	}
        	if(!repeat) num++;
        }
        return num;
    }
    
    /**
     * This method returns true if the word selected from the word list satisfies
     * the constraints specified in the project document. Otherwise it returns
     * false.
     * 
     * @param word
     *            the word selected from the chooseWord method
     * @return
     */
    public static boolean isValidWord(String word) {
        boolean result = true;
        for(int i = 0; i<word.length(); i++) {
        	char c = word.charAt(i);
        	int occur = 0;
        	for(int j = 0; j<word.length(); j++) {
        		if(c == word.charAt(j)) {
        			occur++;
        			if(occur>=3) result = false;
        		}
        	}
        }
        if(word.length()<6) result = false;
        if(word.contains(" ")) result = false;
        return result;
    }

    public static void main(String[] args) throws IOException {
        File f = new File("listOfWords.txt");
        Random rand = new Random(2018);
        Scanner reader = new Scanner(System.in);
        // Do NOT modify the lines above in this method
        
        //global variables
        String word;
        String disguise;
        String letter;
        String play;
        String string = "";
        int guess;
        //start of game
        while(true) {
        	//choose valid word
        	while(!isValidWord(chooseWord(f, rand))) {
            	chooseWord(f, rand);
            }
        	//create a variable called word to store the word chosen
            word = chooseWord(f, rand);
            disguise = createDisguisedWord(word);
            //create a variable called guess to store the number of guesses
            guess = initGuesses(word);
            //ask player to choose the game mode
	        while(true){
		        System.out.println("Choose the game mode (i/I for case insensitive mode, s/S for case sensitive mode): ");
		        letter = reader.nextLine();
		        if(letter.equals("i") || letter.equals("I")) {
		        	System.out.println("You selected case insensitive mode.");
		        	break;
		        }
		        else if(letter.equals("s") || letter.equals("S")) {
		        	System.out.println("You selected case sensitive mode.");
		        	break;
		        }
	        }
	        //main structure of the game
	        while(true) {
	        	System.out.println(disguise+"   "+guess+" "+"remaining guesses.");
	        	System.out.print("Enter a letter or string: ");
	        	if(letter.equals("i") || letter.equals("I")) {
	        		//convert the string input to lower case
	        		string = reader.nextLine().toLowerCase();
	        	}
	        	if(letter.equals("s") || letter.equals("S")) {
	        		string = reader.nextLine();
	        	}
	        	if(isValidGuess(string, word)) {
	        		//update the disguised word
	        		disguise = makeGuess(string, word, disguise);
	        	}
	        	if(!isValidGuess(string, word)) {
	        		System.out.println(string+" does not occur in the word.");
	        		//update the number of guesses
	        		guess--;
	        	}
	        	//tell the player they lost and display the secret word
	        	if(guess == 0) {
	        		System.out.println("You lost! (The word was "+word+")");
	        		break;
	        	}
	        	//congratulate the player and display the secret word
	        	if(isWordRevealed(word, disguise)) {
	        		System.out.println("Great! You saved the man! (The word was "+word+")");
	        		break;
	        	}
	        }
	        while(true) {
	        	//ask player if they want to play again
	        	System.out.println("Do you want to play again? (y/n)");
	        	play = reader.nextLine();
	        	if(play.equals("y") || play.equals("Y") || play.equals("n") || play.equals("N")) {
	        		break;
	        	}
	        }
	        //terminate the game
	        if(play.equals("n") || play.equals("N")) break;
        }
    }
}