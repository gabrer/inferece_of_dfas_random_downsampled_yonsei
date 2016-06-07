/*
 ============================================================================
 Name        : GI-learning.cpp
 Author      : Pietro Cottone
 Version     :
 Copyright   : You can do whatever you want with this code, if you cite me ;)
 Description : Hello World in C++,
 ============================================================================
 */

#include <ctime>
#include <iostream>
#include <string>
#include "dfa.h"
#include "edsm.h"
#include "messages.h"
#include <boost/filesystem.hpp>
#include "lstar.h"
#include "blueStar.h"
#include "bluefringe.h"
#include "geoExp.h"


#define SAMPLE_DIR "examples" 								// training set: put your training data here

#define EDSM_FILE "examples.txt" 							// training set: put your training data here
#define EDSM_FILE_BIG "examples_big.txt" 				// training set: put your training data here

#define LSTAR_FILE "lstar.txt"										// file for lstar
#define LSTAR_FILE_BIG "lstar_big.txt" 						// file for lstar

#define DOT_DIR "results"											// dot file with inferred DFA
#define DOT_FILE_BLUESTAR "inferredDFA_bluestar.dot"
#define DOT_FILE_EDSM "inferredDFA_edsm.dot"
#define DOT_FILE_LSTAR "inferredDFA_lstar.dot"
//#define EDSM_RES_PATH "DFA_dot" 							// by-products of inference


#define MAX_ARGC 4
#define MIN_ARGC 4

namespace fs=boost::filesystem;

using namespace std;


void parse_input_args(int, char**, int&, int&, string *);


int main(int argc, char* argv[]){

	clock_t tStart;

	//file
	string db_path="";

	// working dir
	string base_path;
	string res_path;

	int 	user = -1;
	int		train_prop = -1;

	//parse input arguments
	parse_input_args(argc, argv, user, train_prop, &db_path);
	cout << "DB path is: "<<db_path<<endl;

	cout << "User: "<< user << endl;


	////////// First Experiment
	cout << endl << endl << "///////////////////////////////////////////////////////////////////////////////////////////////"<<endl;
	cout << "///////////////////////////////////////////////////////////////////////////////////////////////"<<endl;
	cout << "///////////////////////////////////////////////////////////////////////////////////////////////"<<endl;
	cout << "Esperimento 0.01"<<endl;
	cout << "///////////////////////////////////////////////////////////////////////////////////////////////"<<endl;
	cout << "///////////////////////////////////////////////////////////////////////////////////////////////"<<endl;
	cout << "///////////////////////////////////////////////////////////////////////////////////////////////"<<endl;

	int 	min_l_prefix = 1;
	int		max_l_prefix = 7;				// Ex: se metti 2, il prefisso è lungo 1 mentre la cella è identificata da stringa l=2:avremo quindi prefix: "w" e cella "wy", "w3", etc
	bool 	no_repetitions_inside_strings = true;
	//int		train_prop = 100;
	int		num_of_random_sets = 10;
	bool 	bluestar = true;
	double 	alpha = 0.01; 	//0.05 //0.025
	double 	delta = 1000.0;

	bool 	edsm = true;

	geoExp* myexp = new geoExp(db_path, user, min_l_prefix, max_l_prefix, no_repetitions_inside_strings, train_prop, num_of_random_sets, edsm, bluestar, alpha, delta);

	myexp->run_inference_splitting_users();


//	cout << "Esperimento 0.05"<<endl;

//	cout << "Esperimento 0.025"<<endl;

	return 0;
}







void parse_input_args(int argc, char* argv[], int &user, int &train_prop, string *dp){
	if(argc>MAX_ARGC || argc<MIN_ARGC){
		cerr<<MSG_WRONG_ARGC<<endl;

		exit(EXIT_FAILURE);
	}

	user = stringToint(argv[1]);
	cout << "utente: "<< user << endl;

	train_prop = stringToint(argv[2]);
	cout << "Training proportion: "<<train_prop << endl;

	*dp = argv[3];

}
