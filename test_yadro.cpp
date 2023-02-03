#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <set>
#include <regex>

using namespace std;
using Cell = pair<string, int>; // cell addres (name_col and num_row)
using Formula = tuple<Cell, char, Cell>; // for expression (cell_1, operation, cell_2 )
enum valueType {column, number, formula};

void get_column_names(ifstream*, vector<string>*); 
int check_value(string);
void parse_lines(ifstream*, vector<string>*, vector<int>*, map<Cell, int>*, map<Cell, Formula>*);
Formula parse_formula(string);
void calculate_values(map<Cell, int>*, map<Cell, Formula>*);
int calculate(int, int, char);
void print_result(map<Cell, int>*, vector<string>*, vector<int>*);


int main(int argc, char *argv[]){
	if (argc > 1){
		string path = argv[1];
		ifstream file (path); // open file to read
		vector<string> columns; // vector for name columns
		vector<int> rows;	// vector for number rows
		map<Cell, int> values; // map-container for table values
		map<Cell, Formula> formulas; // map-container for expressions table

		if(file.is_open()){

			try{
				get_column_names(&file, &columns);
				parse_lines(&file, &columns, &rows, &values, &formulas);
				calculate_values(&values, &formulas);
				print_result(&values, &columns, &rows);
			}
			catch(const char *msg){
				cout << msg << endl;
			}
		}

		file.close(); // close file

	} else {
		cout << "pass the filename as argument" << endl;
	}

    return 0;
}

void get_column_names(ifstream* f, vector<string>* v){
	/*
	function  description:
		reading first line of file to get column names
		if there are no such elements or column names are entered incorrectly - exception output
	*/
	string temp, cols;
	size_t pos;
	bool empty_cell = false;
	set<string> columns;

	getline(*f, cols, '\n');	

	if (cols.empty()){
		throw "Empty line";
	}

	while(pos != string::npos){
		pos = cols.find(",");
		if (pos != string::npos){
			temp = cols.substr(0, pos); // write to temp column name
			cols.erase(0, pos+1);	// deleting from a string the column name

			if(!empty_cell && pos == 0){	// to skip the first value in the table
				empty_cell = true;
				continue;
			}
		}
		else{
			temp = cols;
		}

		check_value(temp)==valueType::column;	// check correct column name
		v->push_back(temp);	
	}
}

int check_value(string str){
	/*
		check correct data using regular expression
	*/
	regex row("[1-9][0-9]*");
	regex number("-?[1-9][0-9]*|0");
	regex formula("=[a-zA-Z]+[1-9][0-9]*[///+-//*][a-zA-Z]+[1-9][0-9]*");
	regex column("[a-zA-Z]+");

	if (regex_match(str, column)){
		return valueType::column;
	}
	else if(regex_match(str, number)){
		return valueType::number;
	}
	else if(regex_match(str, formula)){
		return valueType::formula;
	}
	else{
		throw "Incorrect value in table";
	}
}

void parse_lines(ifstream* f, vector<string>* v_c, vector<int>* v_r, map<Cell, int>* m_v, map<Cell, Formula>* m_f){
	/*
		*m_v - pointer to map-container cell with values
		*m_f - pointer to map-container cell with expression
	function description:
		read csv file, fill vector for number rows
		fill map-containers cells with values and expression
	*/
	string temp, line;
	set<string> rows;
	while(getline(*f, line)){	
		int value, row, count=0;
		bool row_done = false;	// flag for number rows

		if (line.empty()){
			throw "Empty line";
		}
		size_t pos = line.find(",");
		while(pos != string::npos){
			pos = line.find(",");			
			if (pos != string::npos){
				temp = line.substr(0, pos);	// current value of the string
				line.erase(0, pos+1);	// delete from string value to temp

				if(!row_done){
					row_done = true;
					if (check_value(temp) != valueType::number){
						throw "Name row must be a number";
					}
					row = stoi(temp);
					if(row<1){
						throw "Number rows not equal 0 and not less than 0";
					}
	
					v_r->push_back(row);

					count++;
					continue;
				}
			}
			else{
				temp = line;
			}

			switch(check_value(temp)){
				case valueType::number :
					value = stoi(temp);	// string to int
					m_v->insert({{v_c->at(count-1), row}, value});	// addres cell and his value
				break;
				case valueType::formula:
					m_f->insert({{v_c->at(count-1), row}, parse_formula(temp)});	// addres cell and his expression
				break;
				default:
					throw "Incorrect value";
				break;
			}
			count++;
		}
		if(count != v_c->size()+1)
			throw "Number of columns and values are different";
	
    } 
}

Formula parse_formula(string str){
	/*
		parse expression for two arguments and operation
	*/
	int cell1 = 0, cell2, op = 0;
	op = str.find_first_of("+-/*"); // position operation
	for (int i=0; i<str.length(); i++){
		if (str[i]>'0' && str[i]<='9')
		{
			if(cell1==0){
				cell1 = i;
			}
			else if(op!=0){
				cell2 = i;
				break;
			}
		}
	}
	string left_col, right_col;
	int left_row, right_row;

	// addres two cells 
	left_col = str.substr(1, cell1-1);
	right_col = str.substr(op+1, cell2-op-1);
	left_row = stoi(str.substr(cell1, op-cell1));
	right_row = stoi(str.substr(cell2, str.length()-cell2));

	Cell left, right;
	left = make_pair(left_col, left_row);	// the address pair of the first cell in the expression
	right = make_pair(right_col, right_row);	// the address pair of the second cell in the expression
    	
	return make_tuple(left, str[op], right); // tuple of address of first argument, operation, second argument
}

void calculate_values(map<Cell, int>* m_v, map<Cell, Formula>* m_f){
	/*
		iterate through all the expressions in the table and calculate the value if possible
	*/

	char op;
	int res;
	auto it = m_f->begin();	// iterator map-container
	set<Cell> safe;
	stack<Cell> st;	// stack for storing references in recursion

	while(m_f->size()){

		if(st.size()){
			it = m_f->find(st.top());
		}

		if(it == m_f->end()){
			it = m_f->begin();	
		}

		auto arg1 = m_f->find(get<0>(it->second));	// first argument
		auto arg2 = m_f->find(get<2>(it->second));	// second argument

		// checking for recursion
		if(arg1==m_f->end() && arg2==m_f->end()){
			
			auto val1 = m_v->find(get<0>(it->second));	// first value in expression
			auto val2 = m_v->find(get<2>(it->second));	// second value in expression
			op = get<1>(it->second);

			if(val1==m_v->end() || val2==m_v->end()){
				throw "Incorrect reference in an expression";
			}
			res = calculate(val1->second, val2->second, op);

			if (safe.size()){
				safe.erase(it->first);
				st.pop();
			}

			m_v->insert({it->first, res});
			m_f->erase(it++);
		}

		else if(arg1!=m_f->end()){
			if (!safe.insert(arg1->first).second){
				throw "The expression contains cyclic recursion";
			}
			st.push(arg1->first);
		}

		else{
			if (!safe.insert(arg2->first).second){
				throw "The expression contains cyclic recursion";
			}
			st.push(arg2->first);
		}

	}
}

int calculate(int arg1, int arg2, char op){
// calculate expression and return result

	switch(op){
		case '+':
			return arg1+arg2;
		break;
		case '-':
			return arg1-arg2;
		break;
		case '*':
			return arg1*arg2;
		break;
		case '/':
			if (arg2==0){
				throw "Devision by 0";
			}
			return arg1/arg2;
		break;
		default:
			throw "Incorrect operation";
		break;
	}
}

void print_result(map<Cell, int>* m, vector<string>* v_c, vector<int>* v_r){	// print result

	for(auto it=v_c->begin();it!=v_c->end();it++){
		cout<<","<<*it;
	}
	cout << endl;
	for(auto i=v_r->begin();i!=v_r->end();i++){
		cout<<*i;
		for(auto j=v_c->begin();j!=v_c->end();j++){
			cout<<","<< m->find({*j, *i})->second;
		}		
		cout<<endl;
	}
}