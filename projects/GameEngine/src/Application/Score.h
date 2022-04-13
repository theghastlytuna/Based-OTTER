#pragma once
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

struct userScore {
	std::string name;
	float time;
};

class Score
{
public:
	Score();
	~Score();

	std::vector<userScore> readScores();
	void addScore(std::string name, float time);

	std::string _filename;
	std::ofstream _writeFile;
	std::ifstream _readFile;

};