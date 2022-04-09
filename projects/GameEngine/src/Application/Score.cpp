#include "Score.h"

Score::Score()
{
	_filename = "highscores.txt";
	_outFile.open(_filename);
	_inFile.open(_filename);
}

Score::~Score()
{
	_inFile.close();
	_outFile.close();
}

std::vector<userScore> Score::readScores()
{
	std::string input;
	std::vector<userScore> scores;
	userScore newScore;

	if (_inFile.is_open()) {
		while (_inFile >> input) {
			newScore.name = input;
			_inFile >> input;
			newScore.time = std::stof(input);
			scores.push_back(newScore);
		}

		/*
		for (userScore scoregalore : scores) {
			std::cout << scoregalore.name << "|" << scoregalore.time << std::endl;
		}
		*/

		std::sort(scores.begin(), scores.end(), [](userScore a, userScore b)
		{
			return a.time < b.time;
		});
	}
	else {
		std::cout << "Error: Unable to open highscore file. What the fuck did you do?" << std::endl;
	}
}

void Score::addScore(std::string name, float time)
{
	if (_outFile.is_open()) {
		_outFile << name << " " << time;
	}
	else {
		std::cout << "Error: Unable to open highscore file. What the fuck did you do?" << std::endl;
	}
}