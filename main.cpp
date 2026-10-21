#include <iostream>
#include <string>
#include <random>
#include <thread>
#include <chrono>
#include <fstream>
#include <cctype>
#include <format>
#include <utility>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <ftxui/ftxui.hpp> 


using namespace std;
using namespace this_thread;
using namespace chrono;
using json = nlohmann::json;

// simple trim helper
string trim(string s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    if (start == string::npos)
        return "";
    return s.substr(start, end - start + 1);
}

void sleep(int n)
{
    // std::this_thread::sleep_for(std::chrono::milliseconds(100));
    sleep_for(milliseconds(n));
}

void simulateTyping(const string &text, int delay = 16)
{
    for (char c : text)
    {
        cout << c << flush;

        if (isspace(static_cast<unsigned char>(c)))
        {
            sleep(delay / 2);
        }
        else
        {
            sleep(delay);
        }
    }
}

string lowerCaseString(string &text)
{
    for (char &c : text)
    {
        c = tolower(static_cast<unsigned char>(c));
    }
    return text;
}

int randomInt(int from, int to)
{
    // Randomly generate the move
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> distrib(from, to);
    return distrib(gen);
}

vector<double> computeStats(json userData)
{
    const int totalGames = userData.value("totalGames", 0);
    if (totalGames == 0)
    {
        return {0.0, 0.0, 0.0};
    }
    const json stats = userData.contains("stats") && userData["stats"].is_object()
                           ? userData["stats"]
                           : json::object();

    double probR = static_cast<double>(stats.value("rock", 0)) / totalGames;
    double probP = static_cast<double>(stats.value("paper", 0)) / totalGames;
    double probS = static_cast<double>(stats.value("scissors", 0)) / totalGames;

    return {probR, probP, probS};
}

vector<double> computeTransitionFO(string lastMove, json userData)
{
    if (lastMove == "null" || !userData.contains("transitionFO") || !userData["transitionFO"].is_object())
    {
        return {0.0, 0.0, 0.0};
    }

    lastMove = lowerCaseString(lastMove);
    const json transitionData = userData["transitionFO"].contains(lastMove) && userData["transitionFO"][lastMove].is_object()
                                    ? userData["transitionFO"][lastMove]
                                    : json::object();

    const double valR = transitionData.value("rock", 0);
    const double valP = transitionData.value("paper", 0);
    const double valS = transitionData.value("scissors", 0);
    const double totalSum = valR + valP + valS;

    if (totalSum == 0.0)
    {
        return {0.0, 0.0, 0.0};
    }

    double probR = valR / totalSum;
    double probP = valP / totalSum;
    double probS = valS / totalSum;

    // cout << "\n\n\ndebug = computeTransitionFO : ProbR = " << probR << " ProbP = " << probP << " ProbS = " << probS << endl << endl << endl;

    return {probR, probP, probS};
}

vector<double> computeTransitionSO(string lastSecondMove, string lastMove, json userData)
{
    if (lastMove == "null" ||
        lastSecondMove == "null" ||
        !userData.contains("transitionSO") ||
        !userData["transitionSO"].is_object())
    {
        return {0.0, 0.0, 0.0};
    }

    lastMove = lowerCaseString(lastMove);
    lastSecondMove = lowerCaseString(lastSecondMove);
    const json transitionData = (userData["transitionSO"].contains(lastSecondMove) &&
                                 userData["transitionSO"][lastSecondMove].is_object() &&
                                 userData["transitionSO"][lastSecondMove].contains(lastMove) &&
                                 userData["transitionSO"][lastSecondMove][lastMove].is_object())
                                    ? userData["transitionSO"][lastSecondMove][lastMove]
                                    : json::object();

    const double valR = transitionData.value("rock", 0);
    const double valP = transitionData.value("paper", 0);
    const double valS = transitionData.value("scissors", 0);
    const double totalSum = valR + valP + valS;

    if (totalSum == 0.0)
    {
        return {0.0, 0.0, 0.0};
    }

    double probR = valR / totalSum;
    double probP = valP / totalSum;
    double probS = valS / totalSum;

    return {probR, probP, probS};
}

vector<double> combineComputes(vector<double> statsProbs, vector<double> transitionFOProbs, vector<double> transitionSOProbs, json userData)
{
    double statsWeight = userData["methodWeight"].contains("stats") && userData["methodWeight"]["stats"].is_number() ? userData["methodWeight"]["stats"].get<double>() : 0.2;
    double transitionFOWeight = userData["methodWeight"].contains("transitionFirstOrder") && userData["methodWeight"]["transitionFirstOrder"].is_number() ? userData["methodWeight"]["transitionFirstOrder"].get<double>() : 0.4;
    double transitionSOWeight = userData["methodWeight"].contains("transitionSecondOrder") && userData["methodWeight"]["transitionSecondOrder"].is_number() ? userData["methodWeight"]["transitionSecondOrder"].get<double>() : 0.4;

    return {
        ((statsProbs[0] * statsWeight) + (transitionFOProbs[0] * transitionFOWeight) + (transitionSOProbs[0] * transitionSOWeight)),
        ((statsProbs[1] * statsWeight) + (transitionFOProbs[1] * transitionFOWeight) + (transitionSOProbs[1] * transitionSOWeight)),
        ((statsProbs[2] * statsWeight) + (transitionFOProbs[2] * transitionFOWeight) + (transitionSOProbs[2] * transitionSOWeight))};
}

pair<int, string> predictedMove(const vector<double>& probs, string moves[]) {
    double maxVal = *max_element(probs.begin(), probs.end());
    vector<int> tiedIndices;
    for (int i = 0; i < 3; i++)
        if (probs[i] == maxVal) tiedIndices.push_back(i);

    int idx = tiedIndices.size() == 1 ? tiedIndices[0] : tiedIndices[randomInt(0, tiedIndices.size() - 1)]; // the *user's predicted* move, not the counterMove
    return {idx, moves[idx]};
}

pair<string, string> getComputerInput(
    string moves[],
    json userData,
    vector<double> statsProbs,
    vector<double> transitionFOProbs,
    vector<double> transitionSOProbs)
{
    int result;
    int counterResult;
    string method;

    auto isZeroVector = [](const vector<double>& probs) {
        return probs[0] == 0.0 && probs[1] == 0.0 && probs[2] == 0.0;
    };

    if (userData.value("totalGames", 0) > 25)
    {
        vector<double> combineComputeProbs = combineComputes(statsProbs, transitionFOProbs, transitionSOProbs, userData);

        if (!isZeroVector(combineComputeProbs)) {
            result = predictedMove(combineComputeProbs, moves).first;
            method = "combined";
        } else {
            result = randomInt(0, 2);
            method = "random";
        }
    }
    else if (userData.value("totalGames", 0) > 20)
    {
        if (!isZeroVector(transitionSOProbs)) {
            result = predictedMove(transitionSOProbs, moves).first;
            method = "transitionSecondOrder";
        } else if (!isZeroVector(transitionFOProbs)) {
            result = predictedMove(transitionFOProbs, moves).first;
            method = "transitionFirstOrder";
        } else if (!isZeroVector(statsProbs)) {
            result = predictedMove(statsProbs, moves).first;
            method = "stats";
        } else {
            result = randomInt(0, 2);
            method = "random";
        }
    }
    else if (userData.value("totalGames", 0) > 15)
    {
        if (!isZeroVector(transitionFOProbs)) {
            result = predictedMove(transitionFOProbs, moves).first;
            method = "transitionFirstOrder";
        } else if (!isZeroVector(statsProbs)) {
            result = predictedMove(statsProbs, moves).first;
            method = "stats";
        } else {
            result = randomInt(0, 2);
            method = "random";
        }
    }
    else if (userData.value("totalGames", 0) > 10)
    {
        if (!isZeroVector(statsProbs)) {
            result = predictedMove(statsProbs, moves).first;
            method = "stats";
        } else {
            result = randomInt(0, 2);
            method = "random";
        }
    }
    else
    {
        result = randomInt(0, 2);
        method = "random";
    }

    if (result == 2)
        counterResult = 0;
    else
        counterResult = result + 1;

    return {moves[counterResult], method};
}

string getUserInput(const string moves[])
{
    string userInput;

    simulateTyping("⤷ Choose an option: ", 10);
    getline(cin, userInput);
    userInput = trim(userInput);

    // Normalizing User Input
    if (userInput == "1" || userInput == "2" || userInput == "3")
    {
        return moves[stoi(userInput) - 1];
    }
    else if (userInput == "Rock" || userInput == "rock" || userInput == "R" || userInput == "r" || userInput == "Stone" || userInput == "stone")
    {
        return moves[0];
    }
    else if (userInput == "Paper" || userInput == "paper" || userInput == "P" || userInput == "p")
    {
        return moves[1];
    }
    else if (userInput == "Scissors" || userInput == "scissors" || userInput == "S" || userInput == "s" || userInput == "Scissor" || userInput == "scissor")
    {
        return moves[2];
    }
    else if (userInput == "Q" || userInput == "q" || userInput == "Quit" || userInput == "quit")
    {
        return "quit";
    }
    else
    {
        return "null";
    }
}

json readDataFromFile(const string& name)
{
    ifstream f(name + ".json");
    if (f.is_open())
    {
        try
        {
            json data = json::parse(f);
            f.close();
            return data;
        }
        catch (json::parse_error &e)
        {
            f.close();
            return json::object();
        }
    }
    return json::object();
}

void updateGameData(
    json &humanData,
    json &userData,
    string name,
    string userIn,
    string compIn,
    string result,
    string userLastMove,
    string userLastSecondMove,
    string method,
    const vector<double>& statsProbs,
    const vector<double>& transitionFOProbs,
    const vector<double>& transitionSOProbs,
    string moves[])
{
    // Update humans data -----------------------------------------------------------

    humanData["data"] = "humans";

    int totalGames = 0;
    if (humanData.contains("totalGames") && humanData["totalGames"].is_number())
    {
        totalGames = humanData["totalGames"].get<int>();
    }
    humanData["totalGames"] = totalGames + 1;

    if (!humanData.contains("history") || !humanData["history"].is_object())
    {
        humanData["history"] = json::object();
    }
    if (!humanData.contains("stats") || !humanData["stats"].is_object())
    {
        humanData["stats"] = json::object();
    }
    if (!humanData.contains("transitionFO") || !humanData["transitionFO"].is_object())
    {
        humanData["transitionFO"] = json::object();
    }

    int playersCount = 0;
    if (!humanData["history"].contains(name))
    {
        if (humanData.contains("playersCount") && humanData["playersCount"].is_number())
        {
            playersCount = humanData["playersCount"].get<int>();
        }
        humanData["playersCount"] = playersCount + 1;
    }

    if (!humanData["history"].contains(name) || !humanData["history"][name].is_array())
    {
        humanData["history"][name] = json::array();
    }

    json currentHumanRound;
    currentHumanRound["gameId"] = humanData["history"][name].size() + 1;
    currentHumanRound["user"] = userIn;
    currentHumanRound["computer"] = compIn;
    currentHumanRound["result"] = result;

    humanData["history"][name].push_back(currentHumanRound);

    string statsKey = lowerCaseString(userIn);

    if (humanData["stats"].contains(statsKey) && humanData["stats"][statsKey].is_number())
    {
        humanData["stats"][statsKey] = humanData["stats"][statsKey].get<int>() + 1;
    }
    else
    {
        humanData["stats"][statsKey] = 1;
    }

    // Adding or Updating Transition data
    if (userLastMove != "null")
    {
        userLastMove = lowerCaseString(userLastMove);
        userIn = lowerCaseString(userIn);

        // Ensure parent object exists before accessing nested keys
        if (!humanData["transitionFO"].contains(userLastMove))
        {
            humanData["transitionFO"][userLastMove] = json::object();
        }

        if (humanData["transitionFO"][userLastMove].contains(userIn) && humanData["transitionFO"][userLastMove][userIn].is_number())
        {
            humanData["transitionFO"][userLastMove][userIn] = humanData["transitionFO"][userLastMove][userIn].get<int>() + 1;
        }
        else
        {
            humanData["transitionFO"][userLastMove][userIn] = 1;
        }
    }

    // Update user data ----------------------------------------------------
    userData["name"] = name;

    totalGames = 0;
    if (userData.contains("totalGames") && userData["totalGames"].is_number())
    {
        totalGames = userData["totalGames"].get<int>();
    }
    userData["totalGames"] = totalGames + 1;

    if (!userData.contains("history") || !userData["history"].is_array())
    {
        userData["history"] = json::array();
    }

    json currentUserRound;
    currentUserRound["gameId"] = userData["history"].size() + 1;
    currentUserRound["user"] = userIn;
    currentUserRound["computer"] = compIn;
    currentUserRound["result"] = result;
    currentUserRound["method"] = method;

    userData["history"].push_back(currentUserRound);

    // Adding or Updating Stats data
    if (!userData.contains("stats") || !userData["stats"].is_object())
    {
        userData["stats"] = json::object();
    }

    statsKey = lowerCaseString(userIn);
    if (userData["stats"].contains(statsKey) && userData["stats"][statsKey].is_number())
    {
        userData["stats"][statsKey] = userData["stats"][statsKey].get<int>() + 1;
    }
    else
    {
        userData["stats"][statsKey] = 1;
    }

    // Adding or updating transition first order data
    userLastMove = lowerCaseString(userLastMove);
    if (userLastMove != "null")
    {
        // Ensure parent object exists before accessing nested keys
        if (!userData.contains("transitionFO") || !userData["transitionFO"].is_object())
        {
            userData["transitionFO"] = json::object();
        }
        if (!userData["transitionFO"].contains(userLastMove) || !userData["transitionFO"][userLastMove].is_object())
        {
            userData["transitionFO"][userLastMove] = json::object();
        }

        userIn = lowerCaseString(userIn);
        if (userData["transitionFO"][userLastMove].contains(userIn) && userData["transitionFO"][userLastMove][userIn].is_number())
        {
            userData["transitionFO"][userLastMove][userIn] = userData["transitionFO"][userLastMove][userIn].get<int>() + 1;
        }
        else
        {
            userData["transitionFO"][userLastMove][userIn] = 1;
        }
    }

    // Adding or updating transitions second order data
    userLastSecondMove = lowerCaseString(userLastSecondMove);
    userLastMove = lowerCaseString(userLastMove);
    if (userLastMove != "null" && userLastSecondMove != "null")
    {
        if (!userData.contains("transitionSO") || !userData["transitionSO"].is_object())
        {
            userData["transitionSO"] = json::object();
        }
        if (!userData["transitionSO"].contains(userLastSecondMove) || !userData["transitionSO"][userLastSecondMove].is_object())
        {
            userData["transitionSO"][userLastSecondMove] = json::object();
        }
        if (!userData["transitionSO"][userLastSecondMove].contains(userLastMove) || !userData["transitionSO"][userLastSecondMove][userLastMove].is_object())
        {
            userData["transitionSO"][userLastSecondMove][userLastMove] = json::object();
        }

        userIn = lowerCaseString(userIn);
        if (userData["transitionSO"][userLastSecondMove][userLastMove].contains(userIn) && userData["transitionSO"][userLastSecondMove][userLastMove][userIn].is_number())
        {
            userData["transitionSO"][userLastSecondMove][userLastMove][userIn] = userData["transitionSO"][userLastSecondMove][userLastMove][userIn].get<int>() + 1;
        }
        else
        {
            userData["transitionSO"][userLastSecondMove][userLastMove][userIn] = 1;
        }
    }

    // Add or update the methodWeight
    if (!userData.contains("methodWeight") || !userData["methodWeight"].is_object())
    {
        userData["methodWeight"] = json::object();
        userData["methodWeight"]["stats"] = 0.2;
        userData["methodWeight"]["transitionFirstOrder"] = 0.4;
        userData["methodWeight"]["transitionSecondOrder"] = 0.4;
    }

    auto isInformative = [](const vector<double>& probs) {
        return !(probs[0] == probs[1] && probs[1] == probs[2]); // not uniform/all-zero
    };

    unordered_map<string, bool> candidates; // {methodName, matched}
    
    if (isInformative(statsProbs))
        candidates["stats"] = predictedMove(statsProbs, moves).second == userIn;
    if (isInformative(transitionFOProbs))
        candidates["transitionFirstOrder"] = predictedMove(transitionFOProbs, moves).second == userIn;
    if (isInformative(transitionSOProbs))
        candidates["transitionSecondOrder"] = predictedMove(transitionSOProbs, moves).second == userIn;

    // candidates only contains methods that actually had an opinion this round
    int truOpini = 0, falOpini = 0;
    for (auto &[key, val] : candidates) {
        if (val == 1) truOpini++;
        else if (val == 0) falOpini++;
    }
    
    if (truOpini != 0 && falOpini != 0) {
        for (auto &[key, val] : candidates) {
            if (val == 1) {
                userData["methodWeight"][key] = userData["methodWeight"][key].get<double>() + 0.02;
            }
            else if (val == 0) {
                userData["methodWeight"][key] = userData["methodWeight"][key].get<double>() - ((truOpini * 0.02) / falOpini);
            }

            // Bound the values under 0.1 to 0.9            
            double currentWeight = userData["methodWeight"][key].get<double>();

            if (currentWeight < 0.1) {
                double trimVal = 0.1 - currentWeight;
                userData["methodWeight"][key] = 0.1;
                
                for (auto &[k, v] : userData["methodWeight"].items()) {
                    if (k != key) {
                        double otherWeight = v.get<double>() - (trimVal / 2);
                        userData["methodWeight"][k] = otherWeight;
                    }
                }
            }
            else if (currentWeight > 0.9) {
                double trimVal = currentWeight - 0.9;
                userData["methodWeight"][key] = 0.9;
                
                for (auto &[k, v] : userData["methodWeight"].items()) {
                    if (k != key) {
                        double otherWeight = v.get<double>() + (trimVal / 2);
                        userData["methodWeight"][k] = otherWeight;
                    }
                }
            }
        }
    }
}

void saveDataInFiles(json humanData, json userData)
{
    // Opening the files
    string fileName = userData["name"].get<string>() + ".json";
    ofstream userFile(fileName);
    ofstream humanFile("humans.json");

    humanFile << humanData.dump(4) << endl;
    userFile << userData.dump(4) << endl;

    userFile.close();
    humanFile.close();
}

string checkWinner(string user, string computer, string moves[])
{
    // Checking who won the Round
    if (user == computer)
    {
        return "tie";
    }
    else if (user == moves[0] && computer == moves[1])
    {
        return "computer";
    }
    else if (user == moves[0] && computer == moves[2])
    {
        return "user";
    }
    else if (user == moves[1] && computer == moves[0])
    {
        return "user";
    }
    else if (user == moves[1] && computer == moves[2])
    {
        return "computer";
    }
    else if (user == moves[2] && computer == moves[0])
    {
        return "computer";
    }
    else if (user == moves[2] && computer == moves[1])
    {
        return "user";
    }
    else
    {
        return "null";
    }
}

int main()
{
    // Variables and Data containers0
    int userScore = 0;
    int computerScore = 0;
    int rounds = 0;
    string userLastMove = "null";
    string userLastSecondMove = "null";
    string lastResult = "null";
    string lastMethod = "null";
    string userName;
    string userInput;
    string computerInput;
    string moves[3] = {"rock", "paper", "scissors"};
    json humanGameData;
    json userGameData;
    string method;
    bool playedAtLeastOneRound = false;

    // Starting for the game

    simulateTyping("\n==================================\n", 6);
    simulateTyping("| Rock ✊ | Paper 🖐 | Scissors ✌️ |");
    simulateTyping("\n==================================\n", 6);

    sleep(250);

    cout << "\n\033[36m" << flush; // cyan
    simulateTyping("Input Options:\n");
    simulateTyping("[1] Rock     (R / r)\n[2] Paper    (P / p)\n[3] Scissors (S / s)\n[Q] Quit Game\n\n");
    cout << "\033[0m" << flush; // Reset

    simulateTyping("Enter your name: ");
    getline(cin, userName);
    userName = trim(userName);
    lowerCaseString(userName);

    rounds = 0;
    string roundsInput;
    bool validRounds = false;

    while (!validRounds)
    {
        simulateTyping("Number of rounds: ");
        getline(cin, roundsInput);
        try
        {
            rounds = stoi(roundsInput);
            validRounds = true;
        }
        catch (...)
        {
            simulateTyping("\n{Please enter a valid number}\n\n");
        }
    }
    cout << endl;

    humanGameData = readDataFromFile("humans");
    userGameData = readDataFromFile(userName);

    // Main Game Loop
    for (int round = 1; round <= rounds; round++)
    {
        simulateTyping("[Round " + to_string(round) + "/" + to_string(rounds) + "]\n");

        vector<double> statsProbs = computeStats(userGameData);
        vector<double> transitionFOProbs = computeTransitionFO(userLastMove, userGameData);
        vector<double> transitionSOProbs = computeTransitionSO(userLastSecondMove, userLastMove, userGameData);

        userInput = getUserInput(moves);
        tie(computerInput, method) = getComputerInput(moves,  userGameData, statsProbs, transitionFOProbs, transitionSOProbs);

        if (userInput == "quit")
        {
            string input;
            simulateTyping("\n\nAre you sure you want to quit?\n");

            while (true)
            {
                simulateTyping("(Yes/No) Default Y: ");
                getline(cin, input);

                if (input.empty() || input == "Y" || input == "y" || input == "Yes" || input == "yes")
                {
                    simulateTyping("\nThanks for being the part of this game!", 50);
                    break;
                }
                else if (input == "N" || input == "n" || input == "No" || input == "no")
                {
                    break;
                }
                else
                {
                    cout << "\033[33m" << flush; // Color Yellow
                    simulateTyping("\n{Mistake! Please enter Yes/Y or No/N}\n\n");
                    cout << "\033[0m" << flush; // Reset the color
                    continue;
                }
            }

            if (input.empty() || input == "Y" || input == "y" || input == "Yes" || input == "yes")
            {
                break; // actually quit the game
            }
            else
            {
                round--;  // don't consume a round for asking about quitting
                continue; // go back to the top of the round loop
            }
        }

        if (userInput == "null")
        {
            cout << "\033[33m" << flush; // Color Yellow
            simulateTyping(("\n\n{Mistake! Please use valid keyword}\n[1] Rock     (R / r)\n[2] Paper    (P / p)\n[3] Scissors (S / s)\n[Q] Quit Game\n\n\n"));
            cout << "\033[0m" << flush; // Reset the color
            round--;
            continue;
        }

        cout << endl;

        sleep(500);
        simulateTyping("You: " + userInput + "\n", 0);
        simulateTyping("Computer: " + computerInput + "\n", 0);
        sleep(500);

        string result = checkWinner(userInput, computerInput, moves);

        if (result == "user")
        {
            userScore += 10;
            cout << "\033[32m";
            simulateTyping("You Win!!!  (+10)\n");
            cout << "\033[0m";
            sleep(100);
        }
        else if (result == "computer")
        {
            computerScore += 10;
            cout << "\033[31m";
            simulateTyping("Computer Win!  (+10)\n");
            cout << "\033[0m";
            sleep(100);
        }
        else if (result == "tie")
        {
            cout << "\033[33m";
            simulateTyping("It's a Tie\n");
            cout << "\033[0m";
            sleep(100);
        }

        cout << endl;

        cout << "\033[34m";
        simulateTyping("{Score Card:}\n");
        simulateTyping("You: " + to_string(userScore) + "\n");
        simulateTyping("Computer: " + to_string(computerScore) + "\n");
        cout << "\033[0m";

        sleep(100);

        cout << endl
             << endl;

        updateGameData(
            humanGameData,
            userGameData,
            userName,
            userInput,
            computerInput,
            result,
            userLastMove,
            userLastSecondMove,
            method,
            statsProbs,
            transitionFOProbs,
            transitionSOProbs,
            moves);

        playedAtLeastOneRound = true;
        userLastSecondMove = userLastMove;
        userLastMove = userInput;
        lastResult = result;
        lastMethod = method;
    }

    if (userScore > computerScore)
    {
        cout << "\033[32m";
        simulateTyping("\n\n==========\n");
        simulateTyping("YOU WIN :)");
        simulateTyping("\n==========\n\n");
        cout << "\033[0m";
    }
    else if (userScore < computerScore)
    {
        cout << "\033[31m";
        simulateTyping("\n\n==========\n");
        simulateTyping("COMPUTER WINS :(");
        simulateTyping("\n==========\n\n");
        cout << "\033[0m";
    }
    else
    {
        cout << "\033[33m";
        simulateTyping("\n\n=======\n");
        simulateTyping("Tie :|");
        simulateTyping("\n=======\n\n");
        cout << "\033[0m";
    }

    if (playedAtLeastOneRound)
    {
        saveDataInFiles(humanGameData, userGameData);
    }

    return 0;
}



// Common ANSI Color Codes:
// Reset: \033[0m
// Red: \033[31m
// Green: \033[32m
// Yellow: \033[33m
// Blue: \033[34m
// Magenta: \033[35m
// Cyan: \033[36m
