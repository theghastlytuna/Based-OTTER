#include "Score.h"

Score::Score()
{
	_filename = "highscores.txt";
	_writeFile.open(_filename, std::ios::app);
	//_readFile.open(_filename);
}

Score::~Score()
{
	//_readFile.close();
	_writeFile.close();
}

std::vector<userScore> Score::readScores()
{
	std::string input;
	std::vector<userScore> scores;
	userScore newScore;

	_readFile.open(_filename);

	if (_readFile.is_open()) {
		while (_readFile >> input) {
			newScore.name = input;
			_readFile >> input;
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
	_readFile.close();
	return scores;
}

void Score::addScore(std::string name, float time)
{
	if (_writeFile.is_open()) {
		_writeFile << name << " " << time << std::endl;
	}
	else {
		std::cout << "Error: Unable to open highscore file. What the fuck did you do?" << std::endl;
	}
}