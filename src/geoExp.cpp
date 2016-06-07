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
#include <map>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <string>
#include <exception>

#include "geoExp.h"

#define EXP_DIR "experiments"

#define MAX_BUFFER_SIZE 				256

#define USERS_IN_DB						180


#define PREFIX_NUMBER_LIMIT 			1000000				// Eventuale limite nel numero di prefissi analizzati per utente

#define MIN_NUMBER_POSITIVE_SAMPLES  	25
#define MIN_NUMBER_TEST_SAMPLES 		5

#define K_FOLD_CROSS_VAL 				5

//#include <ctime>
#include <chrono>

namespace fs=boost::filesystem;

using namespace std;


// An experiment is charatterizated by a single user
geoExp::geoExp(string db_path, int user, int min_prefixes_length, int max_prefixes_length, bool repetitions, int train_proportion, \
						int num_of_random_sets,  bool alg_edsm, bool alg_blues, double alpha, double delta)
{
	this->db_path = db_path;
	this->user = user;


	if(min_prefixes_length < 0 || min_prefixes_length > 9)
		min_prefixes_length = 1;
	this->min_prefixes_length = min_prefixes_length;

	if(max_prefixes_length <0 || max_prefixes_length > 9)
		max_prefixes_length = 9;
	this->max_prefixes_length = max_prefixes_length;


	this->no_repetitions_inside_strings = repetitions;


	this->num_of_random_sets = num_of_random_sets;


	this->training_proportion = train_proportion;

	this->test_proportion = 100 - train_proportion;



	edsm 		= alg_edsm;
	blueStar 	= alg_blues;
	if(blueStar){
		alpha_value = alpha;
		delta_value = delta;
	}
	else if(alpha != 0.0){
		cerr << "Alpha values selected without BlueStar algorithm"<<endl;
		exit(EXIT_FAILURE);
	}



	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Check or create the root "experiments" folder
	set_root_exp_folders();


	// Create the current experiment folder
	set_current_exp_folder();
}



//TODO: verificare che venga invoca il distruttore della classe "bluefringe"
geoExp::~geoExp(){};



// Find and set the execution folder, it's the base path
void geoExp::set_root_exp_folders()
{
	// Set the main directory: is the parent directory of execution folder
	exec_path = fs::current_path().parent_path().c_str();
	exec_path = exec_path + fs::path::preferred_separator;


	// Create the experiments folder, if not exists
	// From the execution folder, check and create an "experiments" folder.
	// Inside it, there are folders for every different length of the exmperiment
	// There are ".." beacuse exectuion folder is downside the main folder
	if( !fs::exists(exec_path+EXP_DIR) )
		root_exp_path = create_folder(exec_path, EXP_DIR, false);
	else
		root_exp_path = exec_path+EXP_DIR+"/";

	cout << "\"Experiments\" folder checked" << endl;
}


// Find and set the execution folder, it's the base path
void geoExp::set_current_exp_folder()
{

	// Create the current experiment folder
	// From the execution folder, check and create an "experiments" folder.
	// Inside it, there are folders for every different length of the exmperiment
	if(edsm)
		current_exp_folder = create_folder(root_exp_path, "user"+intTostring(user)+"_EDSM", true);
	else if(blueStar)
		current_exp_folder = create_folder(root_exp_path, "user"+intTostring(user)+"_BLUES", true);

	cout << "Current experiment directory is: "<<endl << current_exp_folder<<endl;

}



// Se "current_time" è true crea comunque una nuova cartella con l'orario di invocazione
string geoExp::create_folder(const string  base_path, const string new_folder, bool current_time){

	// Path of the new folder
	string new_path;

	// Define the folder for experiment results, if exists create a folder with modified name with current time
	new_path = base_path + new_folder;


	// move res_dir if it exists
	if(fs::exists(new_path) || current_time)
	{
		char append_time[MAX_BUFFER_SIZE];
		time_t rawtime = std::time(NULL);
		struct tm * timeinfo;
		time ( &rawtime );
		timeinfo = localtime ( &rawtime );
		strftime(append_time, MAX_BUFFER_SIZE, "%m_%d_%H_%M_%S", timeinfo);

		// Check if the folder exist yet, differntly change only the path and don't rename an inexistent folder
		if(fs::exists(new_path))
			fs::rename(new_path, new_path + "_" + append_time);
		else
			new_path = new_path + "_" + append_time;
	}


	// create res_dir
	fs::create_directory( new_path );


	// Update the exp_path for future use
	new_path = new_path + fs::path::preferred_separator;


	return new_path;
}




///////////////////////////////////////////////////////////////////////////////
// Effettua l'inferenza, però divide in 2 l'utente in analisi creandone 2 fittizzi con la metà dei samples.
void geoExp::run_inference_splitting_users()
{
	cout << "Selected algorithms are: " << endl;
	if(blueStar)
		cout << "BlueStar"<<endl;


	cout << "Database path: "<<db_path<<endl;

	cout << "Lunghezza minima dei prefissi: " << min_prefixes_length << endl;


	// UserIDs as string
	vector<string> users = get_userIDs_as_strings_yonsei();
	string current_user = users[user];



	////////////////////////////////////////////////////////
	/////  VARIABLE PREFIX LENGTHS   ////
	////////////////////////////////////////////////////////
	for(int j=min_prefixes_length; j<max_prefixes_length; ++j)
	{
		string folder_current_prefix_len = "";
		set<string>::iterator it;
		int limit_prefixes_number = 0;

		cout << endl << endl << endl;
		cout << "***************************************************" << endl;
		cout << "***************************************************" << endl;
		cout <<  "---------- PREFISSI DI LUNGHEZZA "+intTostring(j)+", utente "+current_user+" -----------"<<endl;
		cout << "***************************************************" << endl;
		cout << "***************************************************" << endl;


		////////////////////////////////////////////////////////
		//////// GET PREFIXES FROM DB /////////
		////////////////////////////////////////////////////////
		// Open connection to DB
		geodb* mydb = new geodb(db_path);
		mydb->connect();

		// Prefixes of length 'j'
		set<string>* prefixes = mydb->get_prefixes_for_user(current_user, j);

		mydb->close();
		delete mydb;

		if(prefixes->size() == 0){
			cout << "No prefixes for this user!" << endl;
			continue;
		}
		////////////////////////////////////////////////////////


		// Make dir for all prefixes of length 'l'
		folder_current_prefix_len = create_folder(current_exp_folder, intTostring(j), false);


		// Cicla i PREFISSI
		cout << endl << prefixes->size() << " prefixes of length " << j <<", limit to ALL";




		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// ALL PREFIXES FOR CURRENT LENGTH
		for (it=prefixes->begin(); it!=prefixes->end() && limit_prefixes_number < PREFIX_NUMBER_LIMIT; ++it)
		{
			cout << endl << endl << endl << "***************************************************" << endl;
			cout <<  "------> Current Prefix: "<<*it << " - Utente "+current_user<< " <--------"<<endl;
			cout << "***************************************************" << endl;
			limit_prefixes_number++;


			geodb*	mydb = NULL;
			string	path_samples = "";
			string	path_training_data = "";
			string 	path_test_data = "";


			/////////////////////////////////////////////////////////
			/////  WRITE MINITRAJECTORIES FILE //
			/////////////////////////////////////////////////////////
			// Open connection to DB
			mydb = new geodb(db_path);
			mydb->connect();

			// Current prefix
			string current_prefix = *it;

			// Path per scrivere le minitraiettorie come samples, per il prefisso considerato
			path_samples		= folder_current_prefix_len + current_prefix;
			path_training_data	= path_samples + "-samples-CV0.txt";
			path_test_data 		= path_samples + "-test_samples-CV0.txt";


			try
			{
				// Scrivi le ministraiettorie su file di testo
				write_minitraj_from_db_like_samples_RANDOM_REDUCED_SAMPLE(current_user, current_prefix, path_samples);
			}
			catch (const char* msg ){
				cout << msg << endl;
				cout << " >-------------------------------< "<< endl;
				mydb->close();
				delete mydb;
				continue;
			}


			mydb->close();
			delete mydb;
			////////////////////////////////////////////////////////




			////////////////////////////////////////////////////////////////////////////////////////////////////////////
			if(blueStar)
			{
				//for(int i=0; i<cross_val_run; ++i)
				for(int i=0; i<num_of_random_sets; ++i)
				{
					// *********************
					// 	 BLUESTAR algorithm
					// *********************
					cout << endl<< "********  BLUESTAR "+current_prefix+"-cv: "+intTostring(i)+" *********" << endl;
					gi::dfa* BLUESTAR_dfa_A;

					path_training_data	= path_samples + "-" + current_user + "-samples-CV"+intTostring(i)+".txt";


					// Read positive and negative samples
					gi::blueStar* bluestar_exe_A = new gi::blueStar(path_training_data.c_str(), alpha_value, delta_value);


					// Start inference
					try
					{
						BLUESTAR_dfa_A = bluestar_exe_A->run(folder_current_prefix_len+current_prefix+"-");
					}
					catch( const char* msg ){
						cout << msg << "; "<<endl;
						cout << " >---------------------------------------< " << endl;
						continue;
					}



					// *********************
					// 	    BLUESTAR  print
					// *********************
					// Create dot figure for the inferred automaton
					string dotBlueStarpath_alf_A = folder_current_prefix_len+current_prefix+"-"+current_user+"A-DOTbluestarALF-CV"+intTostring(i)+".dot";
					BLUESTAR_dfa_A->print_dfa_dot_mapped_alphabet("BlueStarA", dotBlueStarpath_alf_A.c_str());
					cout << "BLUSTAR DFA A path: "<<dotBlueStarpath_alf_A << endl;


					// It prints the inferred automaton in a text file
					string txtBlueStarpath_alf_A = folder_current_prefix_len+current_prefix+"-"+current_user+"A-CV"+intTostring(i)+"-TXTbluestarALF.txt";
					BLUESTAR_dfa_A->print_dfa_in_text_file(txtBlueStarpath_alf_A);
					cout << "BLUSTAR DFA TXT path: "<<txtBlueStarpath_alf_A << endl;



					// free allocated memory
					if(bluestar_exe_A!=NULL)
						delete bluestar_exe_A;
					if(BLUESTAR_dfa_A != NULL)
						delete BLUESTAR_dfa_A;

				} // CROSS VALL FOR
			}
		}

		prefixes->clear();
		delete prefixes;
	}
}





//////////////////////////////
// WRITING FUNCTIONS

// Scrive su file un sottocampionamento dell'insieme di training iniziale
void geoExp::write_minitraj_from_db_like_samples_RANDOM_REDUCED_SAMPLE(string user,string prefix, string path_samples)
{

	vector<string> positive_downsampled_set[num_of_random_sets];


	// Open connection
	geodb* mydb = new geodb(db_path);
	mydb->connect();


	// Positive Samples - minitrajectories for user and prefix
	vector<string>* positive_samples = mydb->get_minitraj_for_prefix_and_user(user, prefix);


	// Negative Samples - minitrajectories for all users, except 'user', and prefix
	vector<string>* negative_samples = mydb->get_minitraj_for_prefix_and_allUsers_EXCEPT_user(user, prefix);


	int dim_positive = positive_samples->size();
	int dim_negative = negative_samples->size();


	if(dim_positive < MIN_NUMBER_POSITIVE_SAMPLES || dim_negative < MIN_NUMBER_POSITIVE_SAMPLES)
	{
		// Free memory
		for(int i=0; i< num_of_random_sets; ++i){
			positive_downsampled_set[i].clear();
		}

		vector<string>().swap(*positive_samples);
		vector<string>().swap(*negative_samples);
		positive_samples->clear();
		negative_samples->clear();
		delete positive_samples;
		delete negative_samples;

		mydb->close();
		delete mydb;

		throw "Size of dataset too small ";
	}


	cout << " - Scrivo su file i samples -" << endl;
	cout << "Prefisso: "<<prefix<<endl;
	cout << "Numero sample positivi: "<<positive_samples->size()<<endl;
	cout << "Numero sample negativi: "<<negative_samples->size()<<endl;



	// ****************************************
	//	EFFETTUO IL DOWNSAMPLING
	// ****************************************

	// Inizzializzo srand
	srand(time(NULL));


	// Randomize order of samples
	random_shuffle(positive_samples->begin(), positive_samples->end());


	// Calcolo la dimensione del training set e del test set
	int dim_positive_donwsampled = ceil( (double) (training_proportion * dim_positive) / (double) 100);


	if(dim_positive == 1)
		dim_positive_donwsampled = 1;


	cout << "Totale positive: "<<dim_positive<<". "<<"Totali negative: "<<dim_negative << endl;
	cout << "SET DOWNSAMPLED: "<<dim_positive_donwsampled << endl;


	if(dim_positive_donwsampled < MIN_NUMBER_TEST_SAMPLES)
	{
		// Free memory
		for(int i=0; i< num_of_random_sets; ++i){
			vector<string>().swap(positive_downsampled_set[i]);
			positive_downsampled_set[i].clear();
		}

		vector<string>().swap(*positive_samples);
		vector<string>().swap(*negative_samples);
		positive_samples->clear();
		negative_samples->clear();
		delete positive_samples;
		delete negative_samples;

		mydb->close();
		delete mydb;

		throw "Size of dataset too small ";
	}



	////////////////////////////////////////////////////////////////////////
	// DOWNSAMPLING
	if(dim_positive_donwsampled != 0)
	{
		for(int i=0; i< num_of_random_sets; ++i)
		{
			random_shuffle(positive_samples->begin(), positive_samples->end());

			//int stop_index = positive_samples->size();

			for(auto w=0; w<dim_positive_donwsampled; ++w)
				positive_downsampled_set[i].push_back(positive_samples->at(w));


			cout << "END: Positive DOWNSAMPLED SET: " << positive_downsampled_set[i].size() <<endl;
		}
	}




	////////////////////////////////////////////////////////////////////////

	// Scrivo su file
	// (è qui dentro che eventualmente tolgo le RIPETIZIONI interne ad una stringa)
	cout << "Scritture su file dei samples..."<<endl;

	for(int i=0; i<num_of_random_sets; ++i)
	{
		// Scrivo il SET DOWNSAMPLE
		string path_training_data = path_samples+ "-"+user+"-samples-CV"+intTostring(i)+".txt";
		write_minitrajectories_as_training_set(&positive_downsampled_set[i] , negative_samples, path_training_data.c_str());

	}


	// Free memory
	for(int i=0; i< num_of_random_sets; ++i){
		vector<string>().swap(positive_downsampled_set[i]);
		positive_downsampled_set[i].clear();
	}

	vector<string>().swap(*positive_samples);
	vector<string>().swap(*negative_samples);
	positive_samples->clear();
	negative_samples->clear();
	delete positive_samples;
	delete negative_samples;

	mydb->close();
	delete mydb;

}



// Scrive su file le minitraiettorie giˆ estratte (p_samples) per l'utente ed un prefisso particolare
void geoExp::write_minitrajectories_as_training_set(vector<string>* p_orig_samples, vector<string>* n_orig_samples, const char * file_path)
{

	// Keep a local copy for working
	vector<string> p_samples(*p_orig_samples);
	vector<string> n_samples(*n_orig_samples);


	// Insert space between single characters in a string. This is the iterator
	vector<string>::iterator it;


	// Positive samples
	for (it=p_samples.begin(); it!=p_samples.end(); ++it)
	{
		// Add spaces between characters
		string new_string = add_space_between_characters_delete_repetitions(*it);
		new_string = trimRight(new_string);

		(*it) = new_string;
	}


	// Negative samples
	int count =0;
	bool non_empty_intersection = false;
	for (it=n_samples.begin(); it!= n_samples.end();)
	{

		// Add spaces between characters
		string new_string = add_space_between_characters_delete_repetitions(*it);
		new_string = trimRight(new_string);


		if( find( p_samples.begin(), p_samples.end(), new_string) != p_samples.end() ){
			//cout << "Doppione tra Sample positivo e negativo: "<<new_string<< endl;
			non_empty_intersection = true;
			it = n_samples.erase(it);									// Per ogni run cambia il numero di volte che entra perché il test set è casuale
		} else {

			(*it) = new_string;
			++it;
		}


		// Add new positive samples to map
		//samples[new_string] = 0;
	}


	cout << "Dopo il aver tolto ripetizioni intrastringhe e tolto samples comuni tra pos e neg, i neg sono: "<<n_samples.size() << endl;

	if(non_empty_intersection)
		cout << "Ci sono stati doppioni tra pos e neg"<<endl;



	//////////////////////////////
	// Add to map with weights for samples
	map<string, int> weights_p;
	map<string, int> weights_n;

	for(auto i=p_samples.begin(); i!=p_samples.end(); ++i){
		auto j = weights_p.begin();

		if((j = weights_p.find(*i)) != weights_p.end())				// Controllo che sia definito uno stato per quando entra dopo lambda un elemento dell'alfabeto
			j->second++;
		else
			weights_p[*i] = 1;
	}

	for(auto i=n_samples.begin(); i!=n_samples.end(); ++i){
		auto j = weights_n.begin();

		if((j = weights_n.find(*i)) != weights_n.end())				// Controllo che sia definito uno stato per quando entra dopo lambda un elemento dell'alfabeto
			j->second++;
		else
			weights_n[*i] = 1;
	}




	// **********************
	//   WRITE FILE SAMPLES
	// **********************

	cout << "Scrittura su file del training set (positivi e negativi)"<<endl;


	// Write file with positive and negative samples
	ofstream myfile;
	myfile.open(file_path);


	// Write alphabet size
	myfile << "32" << "\n";

	// Write Geohash alphabet symbols
	myfile << "$ ";									// Empty symbol
	for(int i=0; i<32; ++i)
		myfile << geo_alph[i] << " ";
	myfile << "\n";

	// Write positive samples
	for (auto it=weights_p.begin(); it!=weights_p.end(); ++it)
		if( it->first.length() < MAX_LENGTH_SAMPLES_POS )
			myfile << "+ "+intTostring(it->second)+" "+it->first+"\n";

	// Write negative samples
	for (auto it=weights_n.begin(); it!=weights_n.end(); ++it)
		if( it->first.length() < MAX_LENGTH_SAMPLES_POS )
			myfile << "- "+intTostring(it->second)+" "+it->first+"\n";


	cout << "Su file sono presenti "<<weights_p.size()<<" samples positivi e "<<weights_n.size()<<" negativi"<<endl;

//	// Write positive samples
//	for(It p1=samples.begin(); p1!=samples.end(); ++p1)
//		if((*p1).second  == 1)
//			if( (*p1).first.size() < MAX_LENGTH_SAMPLES_POS)
//				myfile << "+ "+(*p1).first+"\n";
//
//	// Write negative samples
//	for(It p1=samples.begin(); p1!=samples.end(); ++p1)
//		if((*p1).second  == 0)
//			if( (*p1).first.size() < MAX_LENGTH_SAMPLES_NEG)
//				myfile << "- "+(*p1).first+"\n";


	myfile.close();

}







////////////////////////////
// UTILITIES

// Aggiunge uno spazio tra un carattere e il successivo delle minitraiettorie, per la compatibilità del formato stringhe
string geoExp::add_space_between_characters_delete_repetitions(string old_string)
{
	//cout << "ORIGINALE: " << old_string<<endl;

	string new_string = "";
	int new_length = 0;

	if(!no_repetitions_inside_strings)
	{

		new_length = (  old_string.length() * 2 ) - 1;
		//cout << "Lunghezza nuova stringa: "<<new_length<<endl;

		int index_char = 0;

		char last_char = 'a';

		for(int i=0; i<new_length; ++i)
		{
		   if(i % 2 == 0){

			   new_string = new_string+(old_string)[index_char];
			   last_char = (old_string)[index_char];
			   //cout << "elemento: "<<(old_string)[count]<<endl;
			   index_char++;

		   }else{
			   new_string = new_string+" ";
		   }
		}

	}
	else		// Tolgo qui le RIPETIZIONI interne ad una stringa
	{

		char last_char = 'a';				// Sono sicuro che 'a' non è presente nel geohash

		bool spazio = false;
		for(unsigned int i=0; i<old_string.length(); ++i)
		{
			if(!spazio)
			{
			   if(last_char == (old_string)[i])
				   continue;

			   new_string = new_string+(old_string)[i];
			   last_char = (old_string)[i];

			   spazio = true;
		   }else{
			   new_string = new_string+" ";
			   spazio = false;
		   }
		}
	}

	 //cout << "Nuova stringa: "<<new_string<<endl;

	return new_string;
}



vector<string> geoExp::get_userIDs_as_strings_geolife()
{
	vector<string> users;
	// Utenti analizzati
	for(int i=0; i<USERS_IN_DB; ++i){
		if(i <10)
			users.push_back( "00"+intTostring(i));
		else if(i>=10 && i<100)
			users.push_back("0"+intTostring(i));
		else if(i>=100)
			users.push_back(intTostring(i));
	}

	return users;
}



vector<string> geoExp::get_userIDs_as_strings_yonsei()
{
	vector<string> users;
	// Utenti analizzati
	for(int i=0; i<USERS_IN_DB; ++i)
			users.push_back(intTostring(i));


	return users;
}

