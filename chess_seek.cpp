#define NOMINMAX // Для #include <windows.h>
#define _CRT_SECURE_NO_WARNINGS // Для отключения предупреждений безопасности
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <memory>
#include <algorithm>
#include <map>
#include <cmath>
#include <limits>
#include <iomanip>
#include <random>
#include <sstream>
#include <locale>
#include <codecvt>
#include <chrono>
#include <filesystem> // Убедитесь, что используется C++17
#include <ctime>
#include <set>
#include <unordered_map>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#endif

using namespace std;
namespace fs = std::filesystem;


struct MatchRecord
{
    string opponent_name;
    double opponent_rating;
    string result; // "Win", "Loss", "Draw"
    string date;

    MatchRecord(const string& name, double rating, const string& res, const string& dt)
        : opponent_name(name), opponent_rating(rating), result(res), date(dt) {}
};

string get_current_date()
{
    auto now = chrono::system_clock::now();
    time_t time = chrono::system_clock::to_time_t(now);
    tm tm;
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
    stringstream ss;
    ss << put_time(&tm, "%Y-%m-%d");
    return ss.str();
}

class ChessPlayer
{
public:
    string name;
    string surname;
    string maxRatingDate;
    double rating;
    int wins;
    int draws;
    int defeats;
    bool active;
    bool byed;
    double tournamentpoints = 0.0;
    int tournamentWins;
    int tournamentDraws;
    int tournamentDefeats;
    bool hasBye = false;
    int firstPlaces;
    int secondPlaces;
    int thirdPlaces;
    int colorBalance = 0;
    int roundAdded = 0;
    int tournamentByes = 0;
    double berger;
    double tournamentberger = 0.0;
    double maxRating;
    double initialRating;
    bool hasReceivedBye = false;
    vector<pair<ChessPlayer*, double>> matchHistory;
    vector<MatchRecord> match_history;
    vector<bool> lastColors;
    set<int> reachedMilestones;
    set<pair<string, string>> playedPairs;

public:
    ChessPlayer(const string& n, const string& s, double r, int w = 0, int d = 0, int def = 0,
        bool act = true, int first = 0, int second = 0, int third = 0, int cb = 0)
        : name(n), surname(s), rating(r), initialRating(r), maxRating(r), maxRatingDate(get_current_date()),
        wins(w), draws(d), defeats(def), active(act), colorBalance(cb),
        berger(0.0), byed(false), hasBye(false),
        tournamentWins(0), tournamentDraws(0), tournamentDefeats(0),
        firstPlaces(first), secondPlaces(second), thirdPlaces(third) {}

    void updateMaxRating()
    {
        if (rating > maxRating)
        {
            maxRating = rating;
            maxRatingDate = get_current_date();
        }

    }

    void writeToFile(ofstream& file) const
    {
        file << name << " " << surname << " " << rating << " " << maxRating << " "
            << maxRatingDate << " " << wins << " " << draws << " " << defeats << " "
            << active << " " << firstPlaces << " " << secondPlaces << " " << thirdPlaces << " "
            << colorBalance << endl;
    }

    string getSurname() const { return surname; }
    string getName() const { return name; }
    double getRating() const { return rating; }
    void setRating(double newRating)
    {
        double oldRating = rating;
        rating = newRating;
        updateMaxRating();

        if (newRating > oldRating)
        {
            // Определяем все пройденные сотни между старым и новым рейтингом
            int startMilestone = static_cast<int>(floor(oldRating / 100)) * 100;
            int endMilestone = static_cast<int>(floor(newRating / 100)) * 100;

            // Проверяем каждую сотню между старым и новым рейтингом
            for (int m = startMilestone + 100; m <= endMilestone; m += 100)
                if (m >= 1500 && !reachedMilestones.count(m))
                {
                    reachedMilestones.insert(m);
                    recordMilestone(m);
                }
        }
    }
    double getPoints() const { return wins + draws * 0.5; }
    void addWin() { tournamentWins++; }         // Было: wins++; tournamentWins++;
    void addDraw() { tournamentDraws++; }       // Было: draws++; tournamentDraws++;
    void addDefeat() { tournamentDefeats++; }   // Было: defeats++; tournamentDefeats++;
    void setActive(bool isActive) { active = isActive; }
    bool isActive() const { return active; }
    void resetpoints()
    {
        wins += tournamentWins;
        draws += tournamentDraws;
        defeats += tournamentDefeats;
        berger += tournamentberger;
        tournamentWins = 0;
        tournamentDraws = 0;
        tournamentDefeats = 0;
        tournamentberger = 0.0;
        tournamentByes = 0;
        matchHistory.clear();
        hasBye = false;
    }
    double getTournamentPoints() const { return tournamentWins + tournamentDraws * 0.5 + tournamentByes; }

    void updatePlayerStats(double newRating, int newWins, int newDraws, int newDefeats)
    {
        rating = newRating;
        wins = newWins;
        draws = newDraws;
        defeats = newDefeats;
    }

    int getTotalRounds() const { return wins + draws + defeats; }

    void updateBerger() // Убрать параметр opponents
    {
        double total = 0.0;
        for (const auto& match : matchHistory)
        {
            if (match.first->getSurname() == "BYE") continue;
            ChessPlayer* opponent = match.first;
            double result = match.second;

            if (result == 1.0)
                total += opponent->getTournamentPoints();
            else if (result == 0.5)
                total += opponent->getTournamentPoints() * 0.5;
        }
        tournamentberger = total;
    }


    double getBerger() const { return berger; }
    double getTournamentBerger() const { return tournamentberger; }
    void setBye(bool status) { hasBye = status; }
    bool getBye() const { return hasBye; }

    void addMatchRecord(const MatchRecord& record)
    {
        // Проверка на дубликаты
        auto isDuplicate = [&](const MatchRecord& r)
            {
                return r.opponent_name == record.opponent_name &&
                    r.date == record.date;
            };

        if (find_if(match_history.begin(), match_history.end(), isDuplicate) == match_history.end())
            match_history.push_back(record);
    }

    const vector<MatchRecord>& getMatchHistory() const
    {
        return match_history;
    }

    void saveMatchHistory() const
    {
        namespace fs = std::filesystem;
        fs::create_directory("match_history");

        ofstream file("match_history/" + surname + "_" + name + ".txt");
        for (const auto& record : match_history)
            file << record.date << " | "
            << record.opponent_name << " (" << record.opponent_rating << ") | "
            << "Result: " << record.result << "\n";
    }
    double getPointsPercentage() const
    {
        return wins + draws + defeats > 0 ? (getPoints() / (wins + draws + defeats)) * 100 : 0.0;
    }
    void setReceivedBye(bool status) { hasReceivedBye = status; }
    bool getReceivedBye() const { return hasReceivedBye; }
    void startTournament() { initialRating = rating; } // Устанавливаем начальный рейтинг при старте турнира
    double getTournamentRatingChange() const { return rating - initialRating; } // Разница между текущим и начальным рейтингом

    void initReachedMilestones()
    {
        reachedMilestones.clear();
        int current = rating;
        for (int m = 1500; m <= current; m += 100)
            if (m >= 1500 && rating >= m)
                reachedMilestones.insert(m);
    }

    void recordMilestone(int milestone)
    {
        ofstream file("rating_milestones.txt", ios::app);
        if (file)
            file << name << "|" << surname << "|" << milestone << "|"
            << maxRatingDate << "\n"; // Используем дату достижения максимума
    }

    const set<int>& getReachedMilestones() const { return reachedMilestones; }

    static void printMilestonesTable(const vector<unique_ptr<ChessPlayer>>& players)
    {
        // Чтение данных из файла
        ifstream file("rating_milestones.txt");
        vector<tuple<string, string, int, string>> milestones;

        if (file.is_open())
        {
            string line;
            while (getline(file, line))
            {
                replace(line.begin(), line.end(), '|', ' ');
                istringstream iss(line);
                string name, surname, date;
                int milestone;
                if (iss >> name >> surname >> milestone >> date)
                    milestones.emplace_back(name, surname, milestone, date);
            }
            file.close();
        }

        // Сортировка по дате
        sort(milestones.begin(), milestones.end(),
            [](const auto& a, const auto& b) {
                return get<3>(a) < get<3>(b);
            });

        // Вывод таблицы
        cout << "\nMilestones History:\n";
        cout << left << setw(20) << "Player"
            << setw(12) << "Milestone"
            << setw(12) << "Date"
            << setw(10) << "Rating\n";
        cout << string(50, '-') << endl;

        for (const auto& [name, surname, milestone, date] : milestones)
        {
            cout << setw(20) << (surname + ", " + name)
                << setw(10) << milestone
                << setw(12) << date
                << setw(10) << milestone << endl;
        }
        cout << string(42, '-') << endl;
    }

    void updateColorHistory(bool isWhite)
    {
        colorBalance += isWhite ? 1 : -1;
    }

    void addPlayedPair(const string& surname1, const string& surname2)
    {
        string key1 = min(surname1, surname2);
        string key2 = max(surname1, surname2);
        playedPairs.insert({ key1, key2 });
    }

    bool hasPlayedWith(const string& surname) const
    {
        string key1 = min(this->surname, surname);
        string key2 = max(this->surname, surname);
        return playedPairs.count({ key1, key2 });
    }


};

class TournamentManager
{
    const vector<unique_ptr<ChessPlayer>>& allPlayers; // Ссылка на основной список игроков
    vector<ChessPlayer*> participants;
    map<pair<string, string>, bool> previousPairs;
    int roundNumber = 0;  // Было ошибочное tournament.roundNumber
    struct TempResult
    {
        ChessPlayer* white;
        ChessPlayer* black;
        double result = -1.0;
        string date;
    };

public:

    void reset();

    void addParticipant(ChessPlayer* player) { participants.push_back(player); }

    ChessPlayer* findCandidateForBye()
    {
        if (participants.size() % 2 == 0) return nullptr;

        // Ищем игрока с наименьшим рейтингом без BYE
        ChessPlayer* candidate = nullptr;
        for (auto* p : participants)
            if (!p->getBye() && (!candidate || p->rating < candidate->rating))
                candidate = p;
        return candidate;
    }

    vector<ChessPlayer*> fixedOrderPlayers; // Фиксированный порядок для Round-Robin

    bool canPlayTogether(ChessPlayer* a, ChessPlayer* b) {
        if (a == b || a == nullptr || b == nullptr)
            return false;
        if (a->hasPlayedWith(b->surname))
            return false;
        return true;
    }

    double pairWeight(ChessPlayer* a, ChessPlayer* b) {
        double weight = 0.0;

        // Приоритет 1: Минимизация разницы очков
        double scoreDiff = abs(a->tournamentpoints - b->tournamentpoints);
        weight -= scoreDiff * 1000;  // Чем меньше разница, тем лучше

        // Приоритет 2: Предотвращение повторных встреч
        if (!a->hasPlayedWith(b->surname)) {
            weight += 500;
        }

        // Приоритет 3: Цветовой баланс
        int colorWeight1 = 0;  // a - белые, b - черные
        if (a->colorBalance <= 0) colorWeight1 += 100;
        if (b->colorBalance >= 0) colorWeight1 += 100;

        int colorWeight2 = 0;  // a - черные, b - белые
        if (a->colorBalance >= 0) colorWeight2 += 100;
        if (b->colorBalance <= 0) colorWeight2 += 100;

        weight += max(colorWeight1, colorWeight2);

        // Приоритет 4: Минимизация разницы рейтингов
        double ratingDiff = abs(a->rating - b->rating);
        weight -= ratingDiff * 10;

        return weight;
    }

    void assignColors(ChessPlayer* a, ChessPlayer* b) {
        // Рассчитываем оба варианта расстановки цветов
        int colorWeight1 = (a->colorBalance <= 0 ? 100 : 0) + (b->colorBalance >= 0 ? 100 : 0);
        int colorWeight2 = (a->colorBalance >= 0 ? 100 : 0) + (b->colorBalance <= 0 ? 100 : 0);

        if (colorWeight1 >= colorWeight2) {
            a->colorBalance++;
            b->colorBalance--;
            a->lastColors.push_back(true);
            b->lastColors.push_back(false);
        }
        else {
            a->colorBalance--;
            b->colorBalance++;
            a->lastColors.push_back(false);
            b->lastColors.push_back(true);
        }
    }

    // ChatGpt интегрировало с dutch.cpp 
    // Более чем работает, но сохранение пока не проверял
    // Пришлось ещё несколько раз править, в итоге была создана функция findGroupPairings. Но в целом - отлично
    bool findGroupPairings(vector<ChessPlayer*>& group, vector<pair<ChessPlayer*, ChessPlayer*>>& resultPairs, vector<bool>& used) {
        int n = group.size();
        if (n % 2 != 0) return false; // Нечётное — невозможно

        // Базовый случай — все использованы
        bool allUsed = true;
        for (bool u : used) 
            if (!u) 
                 allUsed = false; break; 
        if (allUsed) 
            return true;

        for (size_t i = 0; i < n; ++i) 
        {
            if (used[i]) 
                continue;
            for (size_t j = i + 1; j < n; ++j) 
            {
                if (used[j]) 
                    continue;
                if (!canPlayTogether(group[i], group[j])) 
                    continue;

                used[i] = used[j] = true;
                resultPairs.emplace_back(group[i], group[j]);

                if (findGroupPairings(group, resultPairs, used)) 
                    return true;

                // Откат
                resultPairs.pop_back();
                used[i] = used[j] = false;
            }
            break; // Варианты с group[i] исчерпаны — дальше бесполезно
        }
        return false;
    }
    //Голландская система
    vector<pair<ChessPlayer*, ChessPlayer*>> generatePairs()
    {
        vector<pair<ChessPlayer*, ChessPlayer*>> pairs;
        updateBergerAndSort(); // Сортировка участников по очкам и дополнительным критериям

        // Активные игроки
        vector<ChessPlayer*> active;
        for (ChessPlayer* p : participants)
            if (p->isActive())
                active.push_back(p);

        // Сортировка по очкам (от большего к меньшему)
        sort(active.begin(), active.end(), [](ChessPlayer* a, ChessPlayer* b) {
            return a->tournamentpoints > b->tournamentpoints;
            });

        // Группируем по очкам
        map<double, vector<ChessPlayer*>> scoreGroups;
        for (ChessPlayer* p : active)
            scoreGroups[p->tournamentpoints].push_back(p);

        ChessPlayer* byeCandidate = nullptr;
        vector<ChessPlayer*> downfloaters;

        // Обрабатываем группы сверху вниз
        for (auto it = scoreGroups.begin(); it != scoreGroups.end(); ++it)
        {
            vector<ChessPlayer*>& group = it->second;

            // Добавляем к группе флотеров из предыдущих групп
            if (!downfloaters.empty())
            {
                group.insert(group.end(), downfloaters.begin(), downfloaters.end());
                downfloaters.clear();
            }

            vector<pair<ChessPlayer*, ChessPlayer*>> localPairs;
            vector<bool> used(group.size(), false);
            if (findGroupPairings(group, localPairs, used)) 
            {
                for (auto& pr : localPairs) 
                {
                    assignColors(pr.first, pr.second);
                    pairs.push_back(pr);
                    previousPairs[{min(pr.first->surname, pr.second->surname), max(pr.first->surname, pr.second->surname)}] = true;
                }
            }
            else 
            {
                // если не удаётся — флотируем всю группу вниз
                downfloaters.insert(downfloaters.end(), group.begin(), group.end());
                continue;
            }

        }

        // Остались непропаренные после всех групп
        if (downfloaters.size() == 1)
        {
            // Назначаем единственный BYE
            ChessPlayer* p = downfloaters.front();
            p->setBye(true);
            pairs.emplace_back(p, nullptr);
        }
        else if (!downfloaters.empty())
        {
            // Ошибка — кто-то остался непропарен при чётном числе
            cerr << "❌ Ошибка: непропаренные игроки при чётном числе участников:" << endl;
            for (ChessPlayer* p : downfloaters)
                cerr << "- " << p->surname << endl;
        }

        return pairs;
    }




    bool havePlayedBefore(ChessPlayer* a, ChessPlayer* b)
    {
        return a->hasPlayedWith(b->surname) ||
            b->hasPlayedWith(a->surname) ||
            previousPairs.count({ min(a->surname, b->surname), max(a->surname, b->surname) });
    }

    void clearParticipants() { participants.clear(); }
    void startNewRound() { roundNumber = 0; }
    void runRound();
    bool empty() const { return participants.empty(); }
    void clear() { participants.clear(); previousPairs.clear(); }

    bool hasActivePlayers() const
    {
        return any_of(participants.begin(), participants.end(),
            [](ChessPlayer* p) { return p->isActive(); });
    }
    TournamentManager(const vector<unique_ptr<ChessPlayer>>& playersRef)
        : allPlayers(playersRef) {}

    void addPlayerBySurname(const string& surname)
    {
        for (const auto& player : allPlayers)
        {
            if (player->getSurname() == surname)
            {
                auto it = find(participants.begin(), participants.end(), player.get());
                if (it == participants.end())
                {
                    participants.push_back(player.get());
                    player->setActive(true);
                    cout << "Игрок " << surname << " добавлен в турнир." << endl;
                }
                else
                {
                    (*it)->setActive(true);
                    cout << "Игрок " << surname << " уже в турнире и теперь активен." << endl;
                }
                participants.back()->roundAdded = roundNumber; // Учитывается в следующем раунде
                return;
            }
        }
        cout << "Игрок с фамилией " << surname << " не найден." << endl;
    }

    void removePlayerBySurname(const string& surname)
    {
        auto it = find_if(participants.begin(), participants.end(),
            [&](ChessPlayer* p) { return p->getSurname() == surname; });

        if (it != participants.end())
        {
            (*it)->setActive(false);
            // Удаляем все пары с этим игроком из истории
            for (auto pairIt = previousPairs.begin(); pairIt != previousPairs.end(); )
            {
                if (pairIt->first.first == surname || pairIt->first.second == surname)
                    pairIt = previousPairs.erase(pairIt);
                else
                    ++pairIt;
            }
            cout << "Игрок " << surname << " удален из следующих туров." << endl;
        }
        else
            cout << "Игрок " << surname << " не найден в турнире." << endl;
    }

private:
    size_t maxRounds() const
    {
        return participants.size() % 2 == 0
            ? participants.size() - 1
            : participants.size();
    }
    void updateRatings(ChessPlayer* winner, ChessPlayer* loser, double result)
    {
        double baseRate = 0.005;
        double diff = loser->getRating() - winner->getRating();
        double rateAdjustment = 0.001 * (diff / 100.0);
        double totalRate = baseRate + rateAdjustment;

        if (diff < 0 && totalRate < 0.002)
            totalRate = 0.002;

        double change = loser->getRating() * totalRate;
        winner->rating += change;
        winner->updateMaxRating();
        loser->rating -= change;
    }

    void updateRatingsDraw(ChessPlayer* p1, ChessPlayer* p2)
    {
        ChessPlayer* higherRated = (p1->rating >= p2->rating) ? p1 : p2;
        ChessPlayer* lowerRated = (p1->rating < p2->rating) ? p1 : p2;

        double median = (higherRated->rating + lowerRated->rating) / 2.0;
        double change = median * ((higherRated->rating - lowerRated->rating) * 0.00001);

        higherRated->rating -= change;
        lowerRated->rating += change;
        p1->updateMaxRating();
        p2->updateMaxRating();
    }
    void applyMatchResult(ChessPlayer* p1, ChessPlayer* p2, double result, const string& date)
    {
        if (p2) // Игнорируем BYE-пары
        {
            p1->addPlayedPair(p1->surname, p2->surname);
            p2->addPlayedPair(p1->surname, p2->surname);
        }

        double originalRating1 = p1->rating;
        double originalRating2 = p2->rating;

        // revertMatchResult(p1, p2, result); Бред, удалить

        if (result == 1.0)
        {
            p1->addWin();
            p2->addDefeat();
            updateRatings(p1, p2, result);
            p1->matchHistory.emplace_back(p2, 1.0);
            p2->matchHistory.emplace_back(p1, 0.0);
            p1->addMatchRecord(MatchRecord(p2->surname + " " + p2->name, originalRating2, "Win", date));
            p2->addMatchRecord(MatchRecord(p1->surname + " " + p1->name, originalRating1, "Loss", date));
        }
        else if (result == 0.0)
        {
            p2->addWin();
            p1->addDefeat();
            updateRatings(p2, p1, 1.0);
            p1->matchHistory.emplace_back(p2, 0.0);
            p2->matchHistory.emplace_back(p1, 1.0);
            p1->addMatchRecord(MatchRecord(p2->surname + " " + p2->name, originalRating2, "Loss", date));
            p2->addMatchRecord(MatchRecord(p1->surname + " " + p1->name, originalRating1, "Win", date));
        }
        else
        {
            p1->addDraw();
            p2->addDraw();
            updateRatingsDraw(p1, p2);
            p1->matchHistory.emplace_back(p2, 0.5);
            p2->matchHistory.emplace_back(p1, 0.5);
            p1->addMatchRecord(MatchRecord(p2->surname + " " + p2->name, originalRating2, "Draw", date));
            p2->addMatchRecord(MatchRecord(p1->surname + " " + p1->name, originalRating1, "Draw", date));
        }
        originalResults.emplace_back(p1, p2, result, originalRating1, originalRating2);
    }

    void revertMatchResult(ChessPlayer* p1, ChessPlayer* p2, double result, double orig1, double orig2)
    {
        p1->rating = orig1; // Восстанавливаем исходные рейтинги
        p2->rating = orig2;

        // Откат статистики
        if (result == 1.0)
        {
            p1->tournamentWins--;
            p2->tournamentDefeats--;
        }
        else if (result == 0.0)
        {
            p2->tournamentWins--;
            p1->tournamentDefeats--;
        }
        else
        {
            p1->tournamentDraws--;
            p2->tournamentDraws--;
        }

        auto removeMatches = [](ChessPlayer* player, ChessPlayer* opponent)
            {
                auto& history = player->matchHistory;
                history.erase(
                    remove_if(history.begin(), history.end(),
                        [opponent](const pair<ChessPlayer*, double>& m) {
                            return m.first == opponent;
                        }),
                    history.end()
                );
            };
        removeMatches(p1, p2);
        removeMatches(p2, p1);
    }

    void updateBergerAndSort()
    {
        for (auto* p : participants)
            p->updateBerger();

        sort(participants.begin(), participants.end(),
            [](ChessPlayer* a, ChessPlayer* b)
            {
                if (a->getTournamentPoints() != b->getTournamentPoints())
                    return a->getTournamentPoints() > b->getTournamentPoints();
                if (a->getTournamentBerger() != b->getTournamentBerger())
                    return a->getTournamentBerger() > b->getTournamentBerger();
                if (a->roundAdded != b->roundAdded)  // Новый критерий
                    return a->roundAdded < b->roundAdded;
                return a->getRating() > b->getRating();
            });
    }

    void printStandings(int currentRound)
    {
        const string RED = "\033[31m";
        const string YELLOW = "\033[33m";
        const string RESET = "\033[0m";

        cout << "\nCurrent standings after " << currentRound << ":\n";
        cout << left << setw(4) << "Pos" << setw(25) << "Player"
            << setw(10) << "Points" << setw(12) << "Berger" << setw(10) << "Round"
            << setw(10) << "Rating" << setw(15) << "Rating Change\n";
        cout << string(70, '-') << endl;

        // Определяем, кого нужно подсветить
        vector<bool> isYellow(participants.size(), false);
        auto comparePlayers = [](ChessPlayer* a, ChessPlayer* b)
            {
                return a->getTournamentPoints() == b->getTournamentPoints() &&
                    a->getTournamentBerger() == b->getTournamentBerger() &&
                    a->roundAdded == b->roundAdded;
            };

        for (size_t i = 0; i < participants.size(); ++i)
        {
            // Проверяем соседей
            if (i > 0 && comparePlayers(participants[i], participants[i - 1]))
                isYellow[i] = isYellow[i - 1] = true;
            if (i < participants.size() - 1 && comparePlayers(participants[i], participants[i + 1]))
                isYellow[i] = isYellow[i + 1] = true;
        }

        for (size_t i = 0; i < participants.size(); ++i)
        {
            const auto& p = participants[i];
            string color = RESET;

            if (!p->isActive())
                color = RED;
            else if (isYellow[i])  // Используем предварительно вычисленные данные
                color = YELLOW;

            cout << color << setw(4) << i + 1
                << setw(25) << (p->getSurname() + ", " + p->getName())
                << setw(10) << fixed << setprecision(1) << p->getTournamentPoints()
                << setw(12) << fixed << setprecision(2) << p->getTournamentBerger()
                << setw(10) << p->roundAdded << setw(10) << fixed << setprecision(2) << p->getRating()
                << setw(15) << fixed << setprecision(2) << p->getTournamentRatingChange()
                << RESET << endl;
        }
        cout << string(85, '-') << endl;
    }

    void finalizeTournament()
    {
        cout << "\nTournament finished!\n";
        vector<ChessPlayer*> sortedParticipants = participants;
        sort(sortedParticipants.begin(), sortedParticipants.end(),
            [](ChessPlayer* a, ChessPlayer* b) {
                if (a->getTournamentPoints() != b->getTournamentPoints())
                    return a->getTournamentPoints() > b->getTournamentPoints();
                if (a->getTournamentBerger() != b->getTournamentBerger())
                    return a->getTournamentBerger() > b->getTournamentBerger();
                return a->getRating() > b->getRating();
            });

        // Находим группы с одинаковыми показателями
        vector<vector<ChessPlayer*>> tieGroups;
        vector<ChessPlayer*> currentGroup;

        for (size_t i = 0; i < sortedParticipants.size(); ++i)
        {
            if (i == 0)
            {
                currentGroup.push_back(sortedParticipants[i]);
                continue;
            }

            bool sameStats =
                sortedParticipants[i]->getTournamentPoints() == sortedParticipants[i - 1]->getTournamentPoints() &&
                sortedParticipants[i]->getTournamentBerger() == sortedParticipants[i - 1]->getTournamentBerger() &&
                sortedParticipants[i]->roundAdded == sortedParticipants[i - 1]->roundAdded;

            if (sameStats)
            {
                if (currentGroup.empty())
                    currentGroup.push_back(sortedParticipants[i - 1]);
                currentGroup.push_back(sortedParticipants[i]);
            }
            else
            {
                if (currentGroup.size() > 1)
                    tieGroups.push_back(currentGroup);
                currentGroup.clear();
            }
        }

        if (currentGroup.size() > 1)
            tieGroups.push_back(currentGroup);

        // Обрабатываем каждую группу
        for (auto& group : tieGroups)
            resolveArmageddonTie(group);

        // Обновляем медальные позиции после армагеддона
        if (roundNumber >= 5)
        {
            if (participants.size() >= 1)
                participants[0]->firstPlaces++;
            if (participants.size() >= 2)
                participants[1]->secondPlaces++;
            if (participants.size() >= 3)
                participants[2]->thirdPlaces++;
        }
        else
            cout << "Not enough rounds (minimum 5) to award medals.\n";
    }
    bool tryFormPairs(const vector<ChessPlayer*>& available, vector<bool>& used, size_t start, vector<pair<size_t, size_t>>& currentPairs);

    void resolveArmageddonTie(vector<ChessPlayer*>& group)
    {
        cout << "\n=== Armageddon Tie Resolution ===" << endl;
        cout << "Players in tie group:" << endl;
        for (auto* p : group)
            cout << "- " << p->getSurname() << " (" << p->getTournamentPoints() << " points)" << endl;

        cout << "Resolve tie with Armageddon? (y/n): ";
        char choice;
        cin >> choice;
        if (tolower(choice) != 'y') return;

        string surname1, surname2;
        ChessPlayer* white = nullptr;
        ChessPlayer* black = nullptr;

        while (true)
        {
            cout << "Enter white player's surname: ";
            cin >> surname1;
            cout << "Enter black player's surname: ";
            cin >> surname2;

            for (auto* p : group)
            {
                if (p->getSurname() == surname1)
                    white = p;
                if (p->getSurname() == surname2)
                    black = p;
            }

            if (white && black) break;
            cout << "Invalid players! Try again." << endl;
            white = black = nullptr;
        }

        cout << white->getSurname() << " (White) vs " << black->getSurname() << " (Black)" << endl;
        cout << "Enter result (1.0 - white win, 0.5 - draw, 0.0 - black win): ";
        double result;
        while (!(cin >> result) || (result != 1.0 && result != 0.5 && result != 0.0))
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid result! Enter 1.0, 0.5 or 0.0: ";
        }

        // Обновляем порядок в списке участников
        auto& participants = this->participants;
        auto white_pos = find(participants.begin(), participants.end(), white);
        auto black_pos = find(participants.begin(), participants.end(), black);

        // Выводим таблицу по новой
        printStandings(roundNumber + 1);

        if (result == 1.0) // Белые выиграли - порядок сохраняется
            cout << white->getSurname() << " wins! Order remains." << endl;
        else // Черные выиграли или ничья - меняем местами
        {
            if (white_pos < black_pos)
                iter_swap(white_pos, black_pos);
            cout << black->getSurname() << " takes higher position!" << endl;
        }

        // Добавляем запись в историю матчей
        string date = "Armageddon " + ::get_current_date();
        white->addMatchRecord(MatchRecord(black->getSurname() + " " + black->getName(),
            black->getRating(),
            (result == 1.0) ? "Win" : (result == 0.5) ? "Draw" : "Loss",
            date));
        black->addMatchRecord(MatchRecord(white->getSurname() + " " + white->getName(),
            white->getRating(),
            (result == 0.0) ? "Win" : (result == 0.5) ? "Draw" : "Loss",
            date));
    }

    vector<tuple<ChessPlayer*, ChessPlayer*, double, double, double>> originalResults;

};
void TournamentManager::reset()
{
    roundNumber = 0;
    previousPairs.clear(); // Очищаем историю пар только для текущего турнира
    for (auto* p : participants)
    {
        p->resetpoints();
        p->startTournament();
        p->setBye(false);
        p->setReceivedBye(false);
        // Не сбрасываем hasReceivedBye здесь, только при создании нового турнира
    }
}
bool TournamentManager::tryFormPairs(const vector<ChessPlayer*>& available, vector<bool>& used, size_t start, vector<pair<size_t, size_t>>& currentPairs) {
    if (start >= available.size())
        return true; // Все игроки распределены

    if (used[start])
        // Игрок уже использован, переходим к следующему
        return tryFormPairs(available, used, start + 1, currentPairs);

    // Перебираем всех возможных соперников для текущего игрока
    for (size_t j = start + 1; j < available.size(); ++j)
        if (!used[j] && !havePlayedBefore(available[start], available[j]))
        {
            // Пытаемся создать пару
            used[start] = true;
            used[j] = true;
            currentPairs.emplace_back(start, j);


            // Рекурсивно формируем оставшиеся пары
            if (tryFormPairs(available, used, start + 1, currentPairs))
                return true;

            // Откатываем изменения, если рекурсивный вызов не удался
            used[start] = false;
            used[j] = false;
            currentPairs.pop_back();
        }

    // Если не нашли неповторяющихся, пробуем любого доступного
    for (size_t j = start + 1; j < available.size(); ++j)
        if (!used[j])
        {
            used[start] = true;
            used[j] = true;
            currentPairs.emplace_back(start, j);

            if (tryFormPairs(available, used, start + 1, currentPairs))
                return true;

            used[start] = false;
            used[j] = false;
            currentPairs.pop_back();
        }

    // Не удалось найти пару для текущего игрока
    return false;
}

wstring create_separator(const vector<int>& widths,
    const wstring& mid_left,
    const wstring& horizontal,
    const wstring& top_div,
    const wstring& mid_right)
{
    wstringstream ss;
    ss << mid_left;
    for (size_t i = 0; i < widths.size(); ++i)
    {
        for (int j = 0; j < widths[i]; ++j) ss << horizontal;
        if (i != widths.size() - 1) ss << top_div;
    }
    ss << mid_right;
    return ss.str();
}
void saveTournamentResults(const vector<ChessPlayer*>& players)
{
    vector<ChessPlayer*> sorted = players;
    sort(sorted.begin(), sorted.end(), [](ChessPlayer* a, ChessPlayer* b)
        {
            if (a->getTournamentPoints() != b->getTournamentPoints())
                return a->getTournamentPoints() > b->getTournamentPoints();
            if (a->getTournamentBerger() != b->getTournamentBerger())
                return a->getTournamentBerger() > b->getTournamentBerger();
            return a->getRating() > b->getRating();
        });

    const string results_dir = "tournament_results";
    fs::create_directories(results_dir); // Создаём папку, если её нет

    auto now = chrono::system_clock::now();
    time_t time = chrono::system_clock::to_time_t(now);
    tm tm;
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
    stringstream date_ss;
    date_ss << put_time(&tm, "%Y-%m-%d_%H-%M-%S");
    const string filename = results_dir + "/tournament_" + date_ss.str() + ".txt";


    // Сортируем игроков
    sort(sorted.begin(), sorted.end(), [](ChessPlayer* a, ChessPlayer* b)
        {
            if (a->getTournamentPoints() != b->getTournamentPoints())
                return a->getTournamentPoints() > b->getTournamentPoints();
            if (a->getTournamentBerger() != b->getTournamentBerger())
                return a->getTournamentBerger() > b->getTournamentBerger();
            return a->getRating() > b->getRating();
        });

    // Сохраняем в файл
    ofstream file(filename);
    if (file.is_open())
    {
        file << "Tournament Results - " << put_time(&tm, "%Y-%m-%d %H:%M:%S") << "\n\n";
        file << left
            << setw(4) << "Pos"
            << setw(25) << "Player"
            << setw(10) << "Rating"
            << setw(10) << "Points"
            << setw(12) << "Berger"
            << setw(10) << "Wins"
            << setw(10) << "Draws"
            << setw(10) << "Defeats\n";

        file << string(80, '-') << "\n";

        for (size_t i = 0; i < sorted.size(); ++i)
        {
            const auto& p = sorted[i];
            file << left << setw(4) << i + 1
                << setw(25) << (p->surname + " " + p->name)
                << setw(10) << fixed << setprecision(0) << p->rating
                << setw(10) << fixed << setprecision(1) << p->getTournamentPoints()
                << setw(12) << fixed << setprecision(2) << p->getTournamentBerger()
                << setw(10) << p->tournamentWins
                << setw(10) << p->tournamentDraws
                << setw(10) << p->tournamentDefeats << "\n";
        }
        file << "\n[Key: Points = Wins + 0.5*Draws | Berger = Sum of opponents' points]";
        cout << "\nResults saved to: " << filename << endl;
    }
    else
        cerr << "Error saving tournament results!" << endl;
}

void displayInfo(const vector<unique_ptr<ChessPlayer>>& players) {
#ifdef _WIN32
    // Настройка консоли для поддержки цветов и UTF-8
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    // Константы для цветов и символов
    const wstring color_reset = L"\033[0m";
    const wstring red_color = L"\033[31m";
    const wstring blue_color = L"\033[34m";
    const vector<wstring> colors = { L"\033[37m", L"\033[90m" };
    const wstring border_color = L"\033[38;2;0;100;0m";
    // Изменяем цвета для групп 1 и 2
    const vector<wstring> colors_group1 = { L"\033[38;2;255;200;200m", L"\033[38;2;255;150;150m" }; // Светло-красные оттенки
    const vector<wstring> colors_group2 = { L"\033[38;2;200;200;255m", L"\033[38;2;150;150;255m" }; // Светло-синие оттенки
    const wstring color_group3 = L"\033[90m"; // Серый

    // Символы псевдографики
    const wchar_t vertical = L'│';
    const wchar_t horizontal = L'─';
    const wchar_t top_left = L'┌';
    const wchar_t top_right = L'┐';
    const wchar_t bottom_left = L'└';
    const wchar_t bottom_right = L'┘';
    const wchar_t mid_left = L'├';
    const wchar_t mid_center = L'┼';
    const wchar_t mid_right = L'┤';

    // Ширина столбцов
    vector<int> col_widths = { 5, 15, 20, 10, 10, 12, 8, 10, 9, 8, 6, 7, 8, 6, 6, 6 };

    // Рассчет максимального количества раундов
    int max_rounds = 0;
    for (const auto& p : players)
        max_rounds = max(max_rounds, p->getTotalRounds());
    const int threshold = max(1, static_cast<int>(0.4 * max_rounds));

    // Получение последних 4 турниров
    vector<string> tournament_files;
    const string results_dir = "tournament_results";
    if (fs::exists(results_dir))
    {
        for (const auto& entry : fs::directory_iterator(results_dir))
            if (entry.path().extension() == ".txt")
                tournament_files.push_back(entry.path().string());
        sort(tournament_files.rbegin(), tournament_files.rend());
        if (tournament_files.size() > 4) tournament_files.resize(4);
    }

    // Сбор участников последних турниров
    set<string> recent_participants;
    for (const auto& file : tournament_files)
    {
        ifstream in(file);
        string line;
        while (getline(in, line))
        {
            if (line.empty() || line.find("Pos") != string::npos)
                continue;

            // Пропускаем строки с разделителями
            if (line.find("---") != string::npos)
                continue;

            istringstream iss(line);
            string pos;
            iss >> pos; // Пропускаем позицию

            // Извлекаем фамилию и имя
            string surname, name, temp;
            while (iss >> temp)
            {
                // Проверяем, является ли следующее слово числом (рейтинг)
                bool isRating = !temp.empty() && (isdigit(temp[0]) || temp[0] == '-');
                if (isRating) break;

                if (surname.empty())
                    surname = temp;
                else
                    name = temp;
            }

            if (!surname.empty() && !name.empty())
                recent_participants.insert(surname + " " + name);
        }
    }

    // Разделение на три группы
    vector<ChessPlayer> group1, group2, group3;
    for (const auto& p : players)
    {
        const int rounds = p->getTotalRounds();
        const bool in_recent = recent_participants.count(p->surname + " " + p->name);

        if (rounds == 0)
            group3.push_back(*p);
        else if (in_recent)
        {
            // Участвовал в последних турнирах - первая группа
            group1.push_back(*p);
        }
        else if (rounds >= threshold)
            // Набрал достаточно раундов без участия в турнирах
            group1.push_back(*p);
        else
            group2.push_back(*p);
    }

    // Компаратор для сортировки
    auto comparator = [](const ChessPlayer& a, const ChessPlayer& b)
        {
            // Сначала сравниваем по рейтингу
            if (a.rating != b.rating)
                return a.rating > b.rating; // по убыванию

            // При равных рейтингах - по проценту очков
            return a.getPointsPercentage() > b.getPointsPercentage();
        };

    // Сортировка групп
    sort(group1.begin(), group1.end(), comparator);
    sort(group2.begin(), group2.end(), comparator);
    sort(group3.begin(), group3.end(), comparator);

    // Объединение групп
    vector<ChessPlayer> sortedPlayers;
    sortedPlayers.insert(sortedPlayers.end(), group1.begin(), group1.end());
    sortedPlayers.insert(sortedPlayers.end(), group2.begin(), group2.end());
    sortedPlayers.insert(sortedPlayers.end(), group3.begin(), group3.end());

    // Функция создания разделителей
    auto create_separator = [&](wchar_t left, wchar_t center, wchar_t right)
        {
            wstringstream ss;
            ss << border_color << left;
            for (size_t i = 0; i < col_widths.size(); ++i)
            {
                ss << wstring(col_widths[i], horizontal);
                if (i != col_widths.size() - 1) ss << center;
            }
            ss << right << color_reset;
            return ss.str();
        };

    // Отрисовка таблицы
    wcout << create_separator(top_left, L'┬', top_right) << endl;

    // Заголовок таблицы
    wcout << border_color << vertical << color_reset
        << left << setw(5) << L"No." << vertical
        << setw(15) << L"Name" << vertical
        << setw(20) << L"Surname" << vertical
        << right << setw(10) << L"Rating" << vertical
        << setw(10) << L"Max Rating" << vertical
        << setw(12) << L"Max Date" << vertical
        << setw(8) << L"Points" << vertical
        << setw(10) << L"Berger" << vertical
        << setw(9) << L"Points %" << vertical
        << setw(8) << L"Rounds" << vertical
        << setw(6) << L"Wins" << vertical
        << setw(7) << L"Draws" << vertical
        << setw(8) << L"Defeats" << vertical
        << setw(6) << L"1st" << vertical
        << setw(6) << L"2nd" << vertical
        << setw(6) << L"3rd" << vertical << endl;

    wcout << create_separator(mid_left, L'┼', mid_right) << endl;

    const size_t g1_end = group1.size();
    const size_t g2_end = g1_end + group2.size();

    for (size_t i = 0; i < sortedPlayers.size(); ++i)
    {
        const auto& p = sortedPlayers[i];
        const bool is_g1 = i < g1_end;
        const bool is_g2 = i >= g1_end && i < g2_end;

        // Определение цвета всей строки
        wstring color;
        if (is_g1)
        {
            const size_t group_index = i;
            color = (group_index % 2 == 0)
                ? L"\033[38;2;255;200;200m"  // Чётные - светло-красный
                : L"\033[38;2;255;150;150m"; // Нечётные - тёмно-красный
        }
        else if (is_g2)
        {
            const size_t group_index = i - g1_end;
            color = (group_index % 2 == 0)
                ? L"\033[38;2;200;200;255m"  // Чётные - светло-синий
                : L"\033[38;2;150;150;255m"; // Нечётные - тёмно-синий
        }
        else
            color = L"\033[90m"; // Серый

        // Вывод разделителя групп
        if (i == g1_end || i == g2_end)
        {
            wstring line_color = (i == g1_end) ? red_color : blue_color;
            wcout << line_color << create_separator(mid_left, L'┼', mid_right)
                << color_reset << endl;
        }

        // Вывод строки игрока (весь текст в выбранном цвете)
        wcout << color  // Устанавливаем цвет для всей строки
            << vertical
            << left << setw(5) << (to_wstring(i + 1) + L".") << vertical
            << setw(15) << wstring(p.name.begin(), p.name.end()) << vertical
            << setw(20) << wstring(p.surname.begin(), p.surname.end()) << vertical
            << right << setw(10) << fixed << setprecision(2) << p.rating << vertical
            << setw(10) << fixed << setprecision(2) << p.maxRating << vertical
            << setw(12) << wstring(p.maxRatingDate.begin(), p.maxRatingDate.end()) << vertical
            << setw(8) << fixed << setprecision(1) << p.getPoints() << vertical
            << setw(10) << fixed << setprecision(2) << p.getBerger() << vertical
            << setw(8) << fixed << setprecision(1) << p.getPointsPercentage() << L"%" << vertical
            << setw(8) << p.getTotalRounds() << vertical
            << setw(6) << p.wins << vertical
            << setw(7) << p.draws << vertical
            << setw(8) << p.defeats << vertical
            << setw(6) << p.firstPlaces << vertical
            << setw(6) << p.secondPlaces << vertical
            << setw(6) << p.thirdPlaces << vertical
            << color_reset  // Сбрасываем цвет после строки
            << endl;

        if (i != sortedPlayers.size() - 1)
            wcout << create_separator(mid_left, L'┼', mid_right) << endl;
    }

    wcout << create_separator(bottom_left, L'┴', bottom_right) << endl;
}

void loadPlayersFromFile(vector<unique_ptr<ChessPlayer>>& players)
{
    ifstream file("chess_players.txt");
    if (file.is_open())
    {
        string line;
        while (getline(file, line))
        {
            istringstream iss(line);
            string name, surname, maxDate;
            double rating, maxRating;
            int wins, draws, defeats, first = 0, second = 0, third = 0;
            bool active;

            if (iss >> name >> surname >> rating >> maxRating >> maxDate >> wins >> draws >> defeats >> active)
            {
                // Пытаемся прочитать новые поля
                iss >> first;
                iss >> second;
                iss >> third;

                players.push_back(make_unique<ChessPlayer>(name, surname, rating,
                    wins, draws, defeats, active, first, second, third));
                players.back()->initReachedMilestones();
                players.back()->setActive(active);
                players.back()->maxRating = maxRating;
                players.back()->maxRatingDate = maxDate;
                players.back()->initReachedMilestones();
            }
        }
        file.close();
    }
    else
        cout << "No existing players found. Starting fresh." << endl;
}


void writePlayersToFile(const vector<unique_ptr<ChessPlayer>>& players)
{
    ofstream file("chess_players.txt");
    ofstream file2("chess_players2.txt"); // Бэкап. Нахрен, опять все данные снесутся 
    if (file.is_open())
    {
        for (const auto& player : players)
            player->writeToFile(file);
        file.close();
        cout << "Player information has been saved to chess_players.txt" << endl;
    }
    else
        cout << "Error opening file for writing." << endl;
    if (file2.is_open())
    {
        for (const auto& player : players)
            player->writeToFile(file2);
        file2.close();
        cout << "Player information backups saved to chess_players2.txt" << endl;
    }
    else
        cout << "Error opening backup file for writing." << endl;
    for (const auto& player : players)
        player->saveMatchHistory();

}

unique_ptr<ChessPlayer> createChessPlayer()
{
    string name, surname;
    double rating;
    cout << "Enter player's name: ";
    cin.ignore((numeric_limits<streamsize>::max)(), '\n');
    getline(cin, name);
    cout << "Enter player's surname: ";
    getline(cin, surname);
    cout << "Enter player's rating: ";
    while (!(cin >> rating))
    {
        cin.clear();
        cin.ignore((numeric_limits<streamsize>::max)(), '\n');
        cout << "Invalid input. Please enter a number for the rating: ";
    }

    return make_unique<ChessPlayer>(name, surname, rating);
}


void TournamentManager::runRound()
{
    bool continueTournament = true;

    while (continueTournament)
    {
        if (roundNumber >= maxRounds())
        {
            cout << "Maximum number of rounds reached!\n";
            vector<ChessPlayer*> sortedParticipants = participants;
            sort(sortedParticipants.begin(), sortedParticipants.end(),
                [](ChessPlayer* a, ChessPlayer* b) {
                    if (a->getTournamentPoints() != b->getTournamentPoints())
                        return a->getTournamentPoints() > b->getTournamentPoints();
                    if (a->getTournamentBerger() != b->getTournamentBerger())
                        return a->getTournamentBerger() > b->getTournamentBerger();
                    return a->getRating() > b->getRating();
                });

            // Обновление мест
            if (sortedParticipants.size() >= 1)
                sortedParticipants[0]->firstPlaces++;
            if (sortedParticipants.size() >= 2)
                sortedParticipants[1]->secondPlaces++;
            if (sortedParticipants.size() >= 3)
                sortedParticipants[2]->thirdPlaces++;

            saveTournamentResults(participants);
            reset();
            continueTournament = false;
            break;
        }
        cout << "\n=== Round " << (roundNumber + 1) << " ===\n";

        // Обновляем сортировку участников перед генерацией пар
        updateBergerAndSort();

        // Генерация пар
        auto pairs = generatePairs();

        // Разделяем пары на обычные и BYE
        vector<pair<ChessPlayer*, ChessPlayer*>> regularPairs;
        vector<pair<ChessPlayer*, ChessPlayer*>> byePairs;
        for (const auto& pair : pairs)
        {
            if (pair.second == nullptr)
                byePairs.push_back(pair);
            else
                regularPairs.push_back(pair);
        }

        // Создаем карту для быстрого поиска индексов участников
        map<ChessPlayer*, int> playerIndex;
        for (size_t i = 0; i < participants.size(); ++i)
            playerIndex[participants[i]] = i;

        // Сортируем обычные пары по минимальному индексу участников
        sort(regularPairs.begin(), regularPairs.end(),
            [&playerIndex](const pair<ChessPlayer*, ChessPlayer*>& a, const pair<ChessPlayer*, ChessPlayer*>& b) {
                int aIdx1 = playerIndex[a.first];
                int aIdx2 = playerIndex[a.second];
                int aMin = min(aIdx1, aIdx2);

                int bIdx1 = playerIndex[b.first];
                int bIdx2 = playerIndex[b.second];
                int bMin = min(bIdx1, bIdx2);

                return aMin < bMin;
            });

        // Объединяем отсортированные обычные пары и BYE пары
        pairs.clear();
        pairs.insert(pairs.end(), regularPairs.begin(), regularPairs.end());
        pairs.insert(pairs.end(), byePairs.begin(), byePairs.end());

        vector<pair<ChessPlayer*, ChessPlayer*>> validPairs;
        vector<TempResult> tempResults;
        for (auto& pair : pairs)
        {
            if (!pair.second)
                tempResults.push_back({ pair.first, nullptr, 1.0, get_current_date() });
            else
                tempResults.push_back({ pair.first, pair.second, -1.0, get_current_date() });
        }

        // Функция отрисовки пар
        auto printPairs = [&]()
            {
#ifdef _WIN32
                HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
                const string red = "\033[31m";
                const string blue = "\033[34m";
                const string yellow = "\033[33m";
                const string green = "\033[32m";
                const string reset = "\033[0m";

                cout << "\nCurrent pairs:\n";
                int pairNumber = 0;
                bool hasBye = false;

                // Выводим все BYE пары с номерами 0, 1...
                for (const auto& tr : tempResults)
                {
                    if (!tr.black)
                    {
                        cout << " [" << pairNumber << "] ";
#ifdef _WIN32
                        SetConsoleTextAttribute(hConsole, 10);
                        cout << tr.white->getSurname() << " vs BYE";
                        SetConsoleTextAttribute(hConsole, 7);
#else
                        cout << green << tr.white->getSurname() << reset << " vs BYE";
#endif
                        cout << " \t[1.0]" << endl;
                        pairNumber++;
                        hasBye = true;
                    }
                }

                // Настраиваем номер для обычных пар: начинаем с 1, если есть BYE
                pairNumber = hasBye ? 1 : 1;

                // Выводим обычные пары, начиная с 1
                for (const auto& tr : tempResults)
                {
                    if (tr.black)
                    {
                        cout << " [" << pairNumber << "] ";

                        // Определяем цвета
                        string color1 = reset;
                        string color2 = reset;

                        if (tr.result >= 0)
                        {
                            if (tr.result == 1.0)
                            {
                                color1 = red;
                                color2 = blue;
                            }
                            else if (tr.result == 0.0)
                            {
                                color1 = blue;
                                color2 = red;
                            }
                            else
                            {
                                color1 = yellow;
                                color2 = yellow;
                            }
                        }

                        // Выводим пару
                        cout << color1 << tr.white->getSurname() << reset
                            << " vs "
                            << color2 << tr.black->getSurname() << reset;

                        // Результат
                        if (tr.result >= 0)
                            cout << " \t[" << fixed << setprecision(1) << tr.result << "]";

                        cout << endl;
                        pairNumber++;
                    }
                }
            };

        // Основной цикл ввода результатов
        vector<size_t> regularIndices;
        for (size_t i = 0; i < tempResults.size(); ++i)
            if (tempResults[i].black != nullptr)
                regularIndices.push_back(i);

        while (true)
        {
            printPairs();
            cout << "\nEnter pair number to edit (0 to finish): ";
            int choice;
            cin >> choice;

            if (choice == 0)
            {
                // Проверка что все результаты введены
                bool allSet = all_of(tempResults.begin(), tempResults.end(),
                    [](const TempResult& tr) { return tr.result >= 0; });

                if (allSet) break;

                cout << "Not all results entered! Continue? (y/n): ";
                char c;
                cin >> c;
                if (tolower(c) == 'n') continue;
                else break;
            }

            if (choice < 1 || choice > regularIndices.size())
            {
                cout << "Invalid pair number! Valid range: 1-" << regularIndices.size() << endl;
                continue;
            }

            size_t realIndex = regularIndices[choice - 1];
            auto& tr = tempResults[realIndex];

            cout << "Enter result for " << tr.white->getSurname()
                << " vs " << tr.black->getSurname()
                << " (1.0/0.5/0.0): ";
            double result;
            cin >> result;

            if (result != 1.0 && result != 0.5 && result != 0.0)
            {
                cout << "Invalid result!" << endl;
                continue;
            }

            tr.result = result;
        }

        // Применение результатов
        vector<tuple<ChessPlayer*, ChessPlayer*, double, double, double>> originalResults;
        for (auto& tr : tempResults)
        {
            if (tr.black)
            {
                originalResults.emplace_back(
                    tr.white,
                    tr.black,
                    tr.result,
                    tr.white->rating,  // сохраняем исходный рейтинг перед изменением
                    tr.black->rating   // сохраняем исходный рейтинг перед изменением
                );
                applyMatchResult(tr.white, tr.black, tr.result, tr.date);
            }
            else
            {
                tr.white->tournamentByes++;
                tr.white->addMatchRecord(MatchRecord("BYE", 0.0, "Win", tr.date));
            }
        }

        // Цикл исправления результатов
        bool needCorrection = true;
        while (needCorrection)
        {
            updateBergerAndSort();
            printStandings(roundNumber + 1);

            cout << "\nDo you want to correct any results? (y/n): ";
            char choice;
            cin >> choice;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            if (tolower(choice) == 'y')
            {
                cout << "Enter pair number to correct: ";
                int pairNumber;
                cin >> pairNumber;

                auto& pair = validPairs[pairNumber - 1];
                auto it = find_if(originalResults.begin(), originalResults.end(),
                    [&pair](const auto& entry) {
                        return get<0>(entry) == pair.first && get<1>(entry) == pair.second;
                    });

                if (it == originalResults.end())
                {
                    cout << "Pair not found!" << endl;
                    continue;
                }

                // Откатываем старый результат
                auto& [p1, p2, oldRes, orig1, orig2] = *it;
                revertMatchResult(p1, p2, oldRes, orig1, orig2);

                // Ввод нового результата
                cout << "Enter new result: ";
                double newResult;
                cin >> newResult;

                // Обновляем исходные данные для будущих откатов
                orig1 = p1->rating;
                orig2 = p2->rating;
                get<2>(*it) = newResult;

                // Применяем новый результат
                applyMatchResult(p1, p2, newResult, get_current_date());
            }
            else
                needCorrection = false;
        }

        // Делаем выбор после раунда
        bool nextRound = false;
        while (!nextRound)
        {
            cout << "\nВыберите действие: (C) Продолжить, (A) Добавить игрока, (R) Удалить игрока, (Q) Выход: ";
            char action;
            cin >> action;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            switch (tolower(action))
            {
            case 'a':
            {
                string surname;
                cout << "Введите фамилию игрока для добавления: ";
                getline(cin, surname);
                addPlayerBySurname(surname);
                printStandings(roundNumber + 1);
                break;
            }
            case 'r':
            {
                string surname;
                cout << "Введите фамилию игрока для удаления: ";
                getline(cin, surname);
                removePlayerBySurname(surname);
                printStandings(roundNumber + 1);
                break;
            }
            case 'c':
                nextRound = true; // Выходим из цикла действий
                continueTournament = true; // Продолжаем турнир
                roundNumber++; // Переход к следующему раунду
                break;
            case 'q':
                nextRound = true;
                continueTournament = false; // Завершаем турнир
                break;
            default:
                cout << "Неверный выбор. Повторите ввод." << endl;
                break;
            }
        }

        if (!continueTournament)
        {
            finalizeTournament();
            saveTournamentResults(participants); // Перемещаем сохранение ПОСЛЕ армагеддона
            reset();
            break;
        }
    }
}

void selectPlayersBySurnames(vector<unique_ptr<ChessPlayer>>& players)
{
    TournamentManager tournament(players);
    vector<string> surnames;
    string surname;
    tournament.clear();
    cout << "Enter surnames to search for one by one. Enter '0' to finish:" << endl;
    cin.ignore((numeric_limits<streamsize>::max)(), '\n');
    // Игнорируем оставшийся символ новой строки от предыдущего ввода.
    while (true)
    {
        cout << "Enter surname: ";
        getline(cin, surname);
        if (surname == "0")
            break; // Завершаем ввод фамилий, при вводе 0
        // Проверяем, существует ли игрок с такой фамилией
        bool found = false;
        for (const auto& player : players)
            if (player->getSurname() == surname)
            {
                found = true;
                break;
            }
        if (!found)
            cout << "Warning!!!: Player with surname '" << surname << "' not found. Please try again." << endl;
        else
            surnames.push_back(surname); // Добавляем фамилию в список, если игрок найден
    }

    if (surnames.empty())
    {
        cout << "No valid surnames were entered. Returning to menu." << endl;
        return;
    }


    tournament.clear(); // Очищаем предыдущих участников
    // Поиск игроков по введённым фамилиям 
    for (const auto& enteredSurname : surnames)
        for (const auto& player : players)
            if (player->getSurname() == enteredSurname)
            {
                tournament.addParticipant(player.get());
                player->setActive(true);
                player->roundAdded = 0; // Активируем игрока
                break;
            }
    if (tournament.empty())
    {
        cout << "No players found with the entered surnames." << endl;
        return;
    }
    if (!tournament.hasActivePlayers())
    {
        cout << "No active players available for the tournament." << endl;
        tournament.reset();
        return;
    }
    tournament.reset();
    cout << "Tournament points and Berger reset for all players." << endl;

    tournament.runRound();

}
void simulateSingleRound(vector<unique_ptr<ChessPlayer>>& players)
{
    string surname1, surname2;
    cout << "Enter first player's surname: ";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(cin, surname1);
    cout << "Enter second player's surname: ";
    getline(cin, surname2);

    ChessPlayer* player1 = nullptr;
    ChessPlayer* player2 = nullptr;

    for (const auto& p : players)
    {
        if (p->surname == surname1)
            player1 = p.get();
        if (p->surname == surname2)
            player2 = p.get();
    }

    if (!player1 || !player2)
    {
        cout << "One or both players not found!" << endl;
        return;
    }

    // Случайный выбор порядка
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> distrib(0, 1);
    if (distrib(gen) == 1)
        swap(player1, player2);

    player1->updateColorHistory(true);  // Белые
    player2->updateColorHistory(false); // Чёрные

    double result;
    cout << "Enter result for " << player1->surname << " vs " << player2->surname << " (1.0/0.5/0.0): ";
    while (!(cin >> result) || (result != 1.0 && result != 0.5 && result != 0.0))
    {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input! Enter 1.0, 0.5 or 0.0: ";
    }

    auto get_current_date = []()
        {
            auto now = chrono::system_clock::now();
            time_t time = chrono::system_clock::to_time_t(now);
            tm tm;
#ifdef _WIN32
            localtime_s(&tm, &time);
#else
            localtime_r(&time, &tm);
#endif
            stringstream ss;
            ss << put_time(&tm, "%Y-%m-%d");
            return ss.str();
        };

    string date = get_current_date();

    if (result == 1.0)
    {
        player1->wins++;
        player2->defeats++;
        // Обновление рейтинга для победы player1
        double baseRate = 0.005;
        double diff = player2->rating - player1->rating;
        double rateAdjustment = 0.001 * (diff / 100.0);
        double totalRate = baseRate + rateAdjustment;
        if (diff < 0 && totalRate < 0.002)
            totalRate = 0.002;
        double change = player2->rating * totalRate;
        player1->rating += change;
        player2->rating -= change;

        player1->addMatchRecord(MatchRecord(player2->surname + " " + player2->name, player2->rating, "Win", date));
        player2->addMatchRecord(MatchRecord(player1->surname + " " + player1->name, player1->rating, "Loss", date));
    }
    else if (result == 0.0)
    {
        player2->wins++;
        player1->defeats++;
        // Обновление рейтинга для победы player2
        double baseRate = 0.005;
        double diff = player1->rating - player2->rating;
        double rateAdjustment = 0.001 * (diff / 100.0);
        double totalRate = baseRate + rateAdjustment;
        if (diff < 0 && totalRate < 0.002)
            totalRate = 0.002;
        double change = player1->rating * totalRate;
        player2->rating += change;
        player1->rating -= change;

        player1->addMatchRecord(MatchRecord(player2->surname + " " + player2->name, player2->rating, "Loss", date));
        player2->addMatchRecord(MatchRecord(player1->surname + " " + player1->name, player1->rating, "Win", date));
    }
    else
    {
        player1->draws++;
        player2->draws++;
        // Обновление рейтинга для ничьи
        ChessPlayer* higherRated = (player1->rating >= player2->rating) ? player1 : player2;
        ChessPlayer* lowerRated = (player1->rating < player2->rating) ? player1 : player2;

        double median = (higherRated->rating + lowerRated->rating) / 2.0;
        double change = median * ((higherRated->rating - lowerRated->rating) * 0.00001);

        higherRated->rating -= change;
        lowerRated->rating += change;

        player1->addMatchRecord(MatchRecord(player2->surname + " " + player2->name, player2->rating, "Draw", date));
        player2->addMatchRecord(MatchRecord(player1->surname + " " + player1->name, player1->rating, "Draw", date));
    }

    writePlayersToFile(players);
    cout << "Match simulated and data saved." << endl;
}

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    setlocale(LC_ALL, "");
    //setlocale(LC_ALL, "Russian"); когда-нибудь добавить поддержку русского языка(в целом, она уже есть. Относительно)
    vector<unique_ptr<ChessPlayer>> players;
    loadPlayersFromFile(players);

#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
#endif

    string choice;
    do {
        cout << "\n1. Create new chess player\n2. Display all players\n3. Select players by surname\n"
            "4. Simulate single round\n5. Show milestones table\n6. Exit\n";
        cout << "Enter your choice: ";
        cin >> choice;

        try {
            int option = stoi(choice);
            switch (option)
            {
            case 1:
                players.push_back(createChessPlayer());
                break;
            case 2:
                displayInfo(players);
                break;
            case 3:
                selectPlayersBySurnames(players);
                break;
            case 4:
                simulateSingleRound(players);
                break;
            case 5:
                ChessPlayer::printMilestonesTable(players);
                break;
            case 6:
                writePlayersToFile(players);
                cout << "Exiting program." << endl;
                exit(0);
            default:
                cout << "Invalid choice. Try again.\n";
            }
        }
        catch (const exception&)
        {
            cout << "Invalid input. Please enter a number between 1-5.\n";
            cin.clear();
            cin.ignore((numeric_limits<streamsize>::max)(), '\n');
        }
    } while (true);
    return 0;
}
