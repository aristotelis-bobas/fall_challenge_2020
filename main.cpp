
/// OPTIMIZATIONS ///

#pragma GCC optimize("Ofast")
#pragma GCC optimize("inline")
#pragma GCC optimize("omit-frame-pointer")
#pragma GCC optimize("unroll-loops")
#pragma GCC option("arch=native", "tune=native", "no-zero-upper")
#pragma GCC target "bmi2"

/// CODINGAME SERVER OPTIMIZATIONS ///

#pragma GCC target("avx")
#include <x86intrin.h>

/////////////////////////////////////

#include <iostream>
#include <string>
#include <map>
#include <list>
#include <vector>
#include <chrono>
#include <cmath>

#define NODE_COUNT 1000000
#define LOG_COUNT 10000
#define INITIAL_DEPTH_LIMIT 15
#define REST 777

#define MIN_SPELLS 12
#define TIMER 47

#define TIER_1_VALUE 1
#define TIER_2_VALUE 2
#define TIER_3_VALUE 3
#define TIER_4_VALUE 4

using namespace std;
using namespace std::chrono;

void processInput();
int getFreeTome();

////////////////////////////////////// CLASSES ///////////////////////////////////

class Benchmark
{
public:
	Benchmark() {}

	void startBenchmark() { start = high_resolution_clock::now(); }
	void endBenchmark()
	{
		end = high_resolution_clock::now();
		time_span = duration<double, std::milli>(end - start);
	}
	auto getResult() { return time_span.count(); }
	void printResult() { cerr << "benchmark took " << time_span.count() << " ms" << endl; }

private:
	high_resolution_clock::time_point start;
	high_resolution_clock::time_point end;
	duration<double, milli> time_span;
};

class Timer
{
public:
	Timer(){};
	static void setTimer() { Timer::start = high_resolution_clock::now(); }
	static void checkEndOfTurn()
	{
		Timer::time_span = duration<double, std::milli>(high_resolution_clock::now() - Timer::start);
		if (Timer::time_span.count() >= TIMER)
			throw "Timer went off.";
	}
	static double getTime()
	{
		time_span = duration<double, std::milli>(high_resolution_clock::now() - start);
		return time_span.count();
	}

private:
	static high_resolution_clock::time_point start;
	static duration<double, milli> time_span;
};

high_resolution_clock::time_point Timer::start;
duration<double, milli> Timer::time_span;

struct Stones
{
	int stones[4];

	Stones() {}
	Stones(vector<int> stones)
	{
		for (int i = 0; i < 4; i++)
			this->stones[i] = stones[i];
	}
	Stones(int blue, int green, int orange, int yellow)
	{
		stones[0] = blue;
		stones[1] = green;
		stones[2] = orange;
		stones[3] = yellow;
	}
};

struct Inventory : public Stones
{
	int score;
	int slots_filled;

	Inventory() {}
	Inventory(int blue, int green, int orange, int yellow, int score)
		: Stones(blue, green, orange, yellow), score(score)
	{
		slots_filled = stones[0] + stones[1] + stones[2] + stones[3];
	}
};

class Action : public Stones
{
public:
	string type;
	int id;

	Action() {}
	Action(int blue, int green, int orange, int yellow, int id)
		: Stones(blue, green, orange, yellow), id(id) {}

	virtual bool haveRequiredStones(const Inventory &inv)
	{
		for (int i = 0; i < 4; i++)
		{
			if (stones[i] < 0 && inv.stones[i] < abs(stones[i]))
				return false;
		}
		return true;
	}
};

class Recipe : public Action
{
public:
	int price;
	int steps = 0;
	float rating = 0;
	list<string> instructions;

	Recipe() {}
	Recipe(int blue, int green, int orange, int yellow, int id, int price)
		: Action(blue, green, orange, yellow, id), price(price) {}

	vector<int> getCost()
	{
		vector<int> cost;

		for (int i = 0; i < 4; i++)
			cost.emplace_back(abs(stones[i]));
		return cost;
	}
};

class Tome : public Action
{
public:
	int tax_cost;
	int tax_gain;
	int tier_based_cost;
	int tier_based_gain;
	int tier_based_nett;
	int gain_spread;
	int cost_spread;
	int gain_depth;
	int cost_depth;
	bool repeat;
	bool freeloader = false;

	Tome() {}
	Tome(int blue, int green, int orange, int yellow, int id, int tax_cost, int tax_gain, bool repeat)
		: Action(blue, green, orange, yellow, id), tax_cost(tax_cost), tax_gain(tax_gain), repeat(repeat)
	{
		tier_based_cost = 0;
		tier_based_gain = 0;

		(stones[0] <= 0) ? tier_based_cost += TIER_1_VALUE * abs(stones[0]) : tier_based_gain += TIER_1_VALUE * stones[0];
		(stones[1] <= 0) ? tier_based_cost += TIER_2_VALUE * abs(stones[1]) : tier_based_gain += TIER_2_VALUE * stones[1];
		(stones[2] <= 0) ? tier_based_cost += TIER_3_VALUE * abs(stones[2]) : tier_based_gain += TIER_3_VALUE * stones[2];
		(stones[3] <= 0) ? tier_based_cost += TIER_4_VALUE * abs(stones[3]) : tier_based_gain += TIER_4_VALUE * stones[3];

		tier_based_nett = tier_based_gain - tier_based_cost;

		gain_spread = 0;
		cost_spread = 0;
		cost_depth = 0;
		gain_depth = 0;

		for (int i = 0; i < 4; i++)
		{
			if (stones[i] > 0)
			{
				gain_spread++;
				gain_depth += stones[i];
			}
			if (stones[i] < 0)
			{
				cost_spread++;
				cost_depth += abs(stones[i]);
			}
		}

		if (cost_spread == 0)
			freeloader = true;
	}

	vector<int> getCost()
	{
		vector<int> cost(4, 0);

		cost[0] = tax_cost;
		return cost;
	}

	bool haveRequiredStones(const Inventory &inv)
	{
		if (inv.stones[0] < tax_cost)
			return false;
		return true;
	}
};

class Spell : public Action
{
public:
	bool avail;
	bool repeat;
	bool repeat_flag = false;
	int repeat_value = 0;
	int tier_based_cost;
	int tier_based_gain;
	int tier_based_nett;

	Spell() {}
	Spell(int blue, int green, int orange, int yellow, int id, bool avail, bool repeat)
		: Action(blue, green, orange, yellow, id), avail(avail), repeat(repeat)
	{
		tier_based_cost = 0;
		tier_based_gain = 0;

		(stones[0] <= 0) ? tier_based_cost += TIER_1_VALUE * abs(stones[0]) : tier_based_gain += TIER_1_VALUE * stones[0];
		(stones[1] <= 0) ? tier_based_cost += TIER_2_VALUE * abs(stones[1]) : tier_based_gain += TIER_2_VALUE * stones[1];
		(stones[2] <= 0) ? tier_based_cost += TIER_3_VALUE * abs(stones[2]) : tier_based_gain += TIER_3_VALUE * stones[2];
		(stones[3] <= 0) ? tier_based_cost += TIER_4_VALUE * abs(stones[3]) : tier_based_gain += TIER_4_VALUE * stones[3];

		tier_based_nett = tier_based_gain - tier_based_cost;
	}

	bool willOverflowInventory(const Inventory &inv, int mult = 1)
	{
		int count = 0;

		for (int i = 0; i < 4; i++)
			count += (stones[i] * mult);
		if (inv.slots_filled + count > 10)
			return true;
		return false;
	}

	bool willOverflowInventory(const array<int, 5> &inv, int mult = 1)
	{
		int count = 0;

		for (int i = 0; i < 4; i++)
			count += (stones[i] * mult);
		if (inv[4] + count > 10)
			return true;
		return false;
	}

	bool haveRequiredStones(const Inventory &inv, int mult = 1)
	{
		for (int i = 0; i < 4; i++)
		{
			if (stones[i] < 0 && inv.stones[i] < mult * abs(stones[i]))
				return false;
		}
		return true;
	}

	bool haveRequiredStones(const array<int, 5> &inv, int mult = 1)
	{
		for (int i = 0; i < 4; i++)
		{
			if (stones[i] < 0 && inv[i] < mult * abs(stones[i]))
				return false;
		}
		return true;
	}
};

////////////////////////////////////// GLOBALS //////////////////////////////////////

class Node;

vector<Node> g_nodes;
vector<vector<int>> g_logs;
map<int, Recipe> g_recipes;
map<int, Tome> g_tomes;
map<int, Spell> g_spells;
Inventory g_inv;

int g_nodes_searched;
int g_solutions_found;
string g_step;

////////////////////////////////////// SPELL UTILITIES  //////////////////////////////////////

bool isRepeatingSpell(int log_id)
{
	for (auto spell : g_spells)
	{
		if (log_id == spell.second.id)
			return false;
	}
	return true;
}

pair<int, int> getRepeatingSpell(int log_id)
{
	for (auto spell : g_spells)
	{
		for (int i = 2; i < 10; i++)
		{
			if (log_id / i == spell.second.id && spell.second.repeat)
				return {spell.second.id, i};
		}
	}
	return {0, 0};
}

////////////////////////////////////// BRUTE FORCE SEARCHER //////////////////////////////////////

/*
	this search tree algorithm brute forces all combinations
	it starts adjusting its own depth limit once a solution is found
*/

class Node
{

public:
	/// object state
	array<int, 5> inv;
	map<int, bool> spells;
	vector<int> log;
	int depth;
	int option_count;
	vector<int> options;
	multimap<int, int> repeats;
	bool rest_option = false;

	/// class state
	static map<int, vector<int>> targets;
	static int limit;
	static int nodes_searched;
	static int target_hits;

	// root_node constructor
	Node(Inventory start_inv, map<int, Spell> search_spells, map<int, vector<int>> search_targets)
	{
		// initialize class state
		Node::targets = search_targets;
		Node::limit = INITIAL_DEPTH_LIMIT;
		Node::nodes_searched = 0;
		Node::target_hits = 0;

		// initialize object state
		inv = {start_inv.stones[0], start_inv.stones[1], start_inv.stones[2], start_inv.stones[3], start_inv.slots_filled};
		for (auto &spell : search_spells)
			spells[spell.first] = spell.second.avail;
		depth = 0;

		// end of chain conditions
		if (targetHit())
			return;
		if (limitHit())
			return;
		Timer::checkEndOfTurn();

		// expansion mechanics
		getOptions();
		increaseDepth();

		g_solutions_found += Node::target_hits;
		g_nodes_searched += Node::nodes_searched;
	}

	// placeholder Node
	Node() {}

	// clean up state of recycled Node
	void cleanNode()
	{
		options.clear();
		repeats.clear();
		option_count = 0;
		rest_option = false;
	}

	// fill recycled Node with appropiate info
	void createChildNode(array<int, 5> mutated_inv, map<int, bool> mutated_spells, vector<int> mutated_log, int mutated_depth)
	{
		cleanNode();

		Node::nodes_searched++;
		inv = mutated_inv;
		spells = mutated_spells;
		log = mutated_log;
		depth = mutated_depth;

		// end of chain conditions
		if (targetHit())
			return;
		Timer::checkEndOfTurn();
		if (limitHit())
			return;

		// expansion mechanics
		getOptions();
		increaseDepth();
	}

	void increaseDepth()
	{
		for (auto spell : repeats)
		{
			g_nodes[Node::nodes_searched].createChildNode(mutatedInventory(spell.first, spell.second), mutatedSpells(spell.first), mutatedLog(spell.first * spell.second), depth + 1);
		}
		for (auto spell_id : options)
		{
			g_nodes[Node::nodes_searched].createChildNode(mutatedInventory(spell_id), mutatedSpells(spell_id), mutatedLog(spell_id), depth + 1);
		}
		if (rest_option)
		{
			map<int, bool> mutated_spells = spells;
			mutateSpellAvailability(mutated_spells);

			g_nodes[Node::nodes_searched].createChildNode(inv, mutated_spells, mutatedLog(REST), depth + 1);
		}
	}

	void getOptions()
	{
		for (auto &spell : spells)
		{
			if (g_spells[spell.first].haveRequiredStones(inv) && !g_spells[spell.first].willOverflowInventory(inv) && spell.second)
				options.push_back(spell.first);
			if (g_spells[spell.first].repeat && spell.second)
			{
				for (int mult = 2; mult < 5; mult++)
				{
					if (g_spells[spell.first].haveRequiredStones(inv, mult) && !g_spells[spell.first].willOverflowInventory(inv, mult))
						repeats.insert({spell.first, mult});
					else
						break;
				}
			}
			if (!rest_option && !spell.second)
				rest_option = true;
		}
		option_count = options.size() + repeats.size();
		if (rest_option)
			option_count++;
	}

	bool limitHit()
	{
		if (depth + 1 > Node::limit)
			return true;
		return false;
	}

	bool targetHit()
	{
		bool solved = false;

		for (auto &target : targets)
		{
			int match = 0;
			for (int i = 0; i < 4; i++)
			{
				if (inv[i] < target.second[i])
					break;
				else
					match++;
			}
			if (match == 4)
			{
				log.emplace_back(target.first);
				g_logs.emplace_back(log);
				solved = true;
				break;
			}
		}
		if (!solved)
			return false;
		if (depth < Node::limit)
			Node::limit = depth;
		Node::target_hits++;
		return true;
	}

	array<int, 5> mutatedInventory(int spell_id, int mult = 1)
	{
		array<int, 5> tmp = inv;

		/// reset filled slots
		tmp[4] = 0;

		for (int i = 0; i < 4; i++)
		{
			(g_spells[spell_id].stones[i] <= 0) ? tmp[i] -= mult * abs(g_spells[spell_id].stones[i]) : tmp[i] += mult * g_spells[spell_id].stones[i];
			tmp[4] += tmp[i];
		}

		return tmp;
	}

	map<int, bool> mutatedSpells(int spell_id)
	{
		map<int, bool> tmp = spells;
		tmp[spell_id] = false;
		return tmp;
	}

	void mutateSpellAvailability(map<int, bool> &spells)
	{
		for (auto &spell : spells)
			spell.second = true;
	}

	vector<int> mutatedLog(int insert)
	{
		vector<int> tmp = log;
		tmp.emplace_back(insert);
		return tmp;
	}
};

// static initializations
map<int, vector<int>> Node::targets;
int Node::limit;
int Node::nodes_searched;
int Node::target_hits;

void initializeNodes()
{
	Benchmark bm;
	bm.startBenchmark();

	g_nodes.reserve(NODE_COUNT);

	for (int i = 0; i < NODE_COUNT; i++)
		g_nodes.emplace_back();

	bm.endBenchmark();
	cerr << "Created global node tree of size " << NODE_COUNT << " in " << bm.getResult() << " ms." << endl;
}

////////////////////////////////////// SEARCH LOGS  //////////////////////////////////////

/// gets optimal log from the previous search
/// retrieves all minimum step options and finds the most gain (based on ingredient tier) / step option

vector<int> getOptimalLog()
{
	vector<int> result;

	// no solutions
	if (g_logs.empty())
		return result;

	// only 1 solution
	if (g_logs.size() == 1)
		return g_logs[0];

	// filter out suboptimal logs
	vector<vector<int>> logs;

	for (auto &log : g_logs)
	{
		if (log.size() - 1 == Node::limit)
			logs.emplace_back(log);
	}

	for (int i = 0; i < Node::limit; i++)
	{
		// getting tier based gain of log current step

		vector<int> gains;

		for (int y = 0; y < logs.size(); y++)
		{
			if (logs[y][i] == REST)
				gains.emplace_back(0);
			else if (isRepeatingSpell(logs[y][i]))
			{
				pair<int, int> instruction = getRepeatingSpell(logs[y][i]);
				if (instruction.first != 0)
					gains.emplace_back(g_spells[instruction.first].tier_based_nett * instruction.second);
				else
					gains.emplace_back(0);
			}
			else
				gains.emplace_back(g_spells[logs[y][i]].tier_based_nett);
		}

		// finding highest tier based gain of current step

		int max_gain = -1234;

		for (auto &gain : gains)
		{
			if (gain > max_gain)
				max_gain = gain;
		}

		// filtering out suboptimal logs

		vector<vector<int>> tmp = logs;
		logs.clear();

		for (int y = 0; y < tmp.size(); y++)
		{
			if (gains[y] == max_gain)
				logs.emplace_back(tmp[y]);
		}

		if (logs.size() == 1)
			break;
	}
	return logs.front();
}

// from all optimal logs for each searched recipe calculates the most profitable next step
// most profitable next step is calculated

string getRecipeStep(vector<int> optimal_log)
{
	if (optimal_log.size() == 1)
	{
		for (auto &recipe : g_recipes)
		{
			if (recipe.second.id == optimal_log.front())
				return "BREW " + to_string(optimal_log.front()) + " HERE'S YOUR SHIT, SIR";
		}
		for (auto &tome : g_tomes)
		{
			if (tome.second.id == optimal_log.front())
				return "LEARN " + to_string(optimal_log.front());
		}
		///// NO SUPPORT FOR REPEATING SPELLS (!)
		for (auto &spell : g_spells)
		{
			if (spell.second.id == optimal_log.front())
				return "CAST " + to_string(optimal_log.front());
		}
	}
	else if (optimal_log.front() == REST)
		return "REST";
	else if (isRepeatingSpell(optimal_log.front()))
	{
		pair<int, int> repeat = getRepeatingSpell(optimal_log.front());
		return "CAST " + to_string(repeat.first) + " " + to_string(repeat.second);
	}
	return "CAST " + to_string(optimal_log.front());
}

string getStep(vector<vector<int>> optimal_logs)
{
	vector<int> optimal_log;
	map<int, float> ratings;
	map<int, vector<int>> recipes;

	if (optimal_logs.empty())
	{
		cerr << "Empty step. Learning free tome instead." << endl;
		return "LEARN " + to_string(getFreeTome()) + " DEEZ NUTS";
	}
	for (auto recipe : optimal_logs)
	{
		int recipe_id = recipe.back();
		recipes[recipe_id] = recipe;
	}

	for (auto recipe : optimal_logs)
	{
		int recipe_id = recipe.back();

		ratings[recipe_id] = pow(g_recipes[recipe_id].price, 2) / (recipe.size() + 1);
		//ratings[recipe_id] = g_recipes[recipe_id].price / (recipe.size() + 1);

		cerr << "Recipe " << recipe_id << " | " << ratings[recipe_id] << " | " << g_recipes[recipe_id].price << " gold | " << recipe.size() << " steps" << endl;
	}
	cerr << "=======================================" << endl;

	float best_rating = -999;
	int best_recipe = 0;
	for (auto rating : ratings)
	{
		if (rating.second > best_rating)
		{
			best_rating = rating.second;
			best_recipe = rating.first;
		}
	}
	cerr << "Target recipe " << best_recipe << " | " << best_rating << " | " << g_recipes[best_recipe].price << " gold | " << recipes[best_recipe].size() << " steps" << endl;
	cerr << "=======================================" << endl;
	return getRecipeStep(recipes[best_recipe]);
}

void addLog(vector<int> log)
{
	g_logs.emplace_back(log);
}

void initializeLogs()
{
	g_logs.reserve(LOG_COUNT);
}

//////////////////////////////////////////////// SEARCHES /////////////////////////////////////////

void searchRecipes()
{
	map<int, vector<int>> search_targets;
	vector<vector<int>> optimal_logs;
	vector<int> recipes_searched;
	vector<int> optimal_log;

	try
	{
		while (true)
		{
			search_targets.clear();
			g_logs.clear();

			for (auto recipe : g_recipes)
			{
				bool already_searched = false;
				for (auto searched : recipes_searched)
				{
					if (recipe.first == searched)
					{
						already_searched = true;
						break;
					}
				}
				if (!already_searched)
					search_targets[recipe.first] = recipe.second.getCost();
			}
			if (search_targets.empty())
				break;
			Node root_node(g_inv, g_spells, search_targets);
			optimal_log = getOptimalLog();
			if (optimal_log.empty())
				break;
			optimal_logs.emplace_back(optimal_log);
			recipes_searched.emplace_back(optimal_logs.back().back());
		}
	}
	catch (const char *e)
	{
		g_solutions_found += Node::target_hits;
		g_nodes_searched += Node::nodes_searched;
		optimal_log = getOptimalLog();
		if (!optimal_log.empty())
			optimal_logs.emplace_back(optimal_log);
	}
	g_step = getStep(optimal_logs);
}

////////////////////////////////////// TOME MECHANICS ///////////////////////////////////

int getFreeTome()
{
	for (auto tome : g_tomes)
	{
		if (tome.second.tax_cost == 0)
			return tome.first;
	}
	return -1;
}

bool hasFreeloader()
{
	for (auto tome : g_tomes)
	{
		if (tome.second.freeloader)
			return true;
	}
	return false;
}

int getFreeloader()
{
	int max_gain = -999;
	vector<int> tmp_tomes;
	int ret = 0;

	for (auto tome : g_tomes)
	{
		if (tome.second.freeloader)
			tmp_tomes.push_back(tome.first);
	}
	for (auto i : tmp_tomes)
	{
		if (g_tomes[i].tier_based_nett >= max_gain)
		{
			if (ret != 0 && g_tomes[i].tier_based_nett == max_gain)
			{
				if (g_tomes[i].gain_spread > g_tomes[ret].gain_spread)
					continue;
			}
			ret = i;
			max_gain = g_tomes[i].tier_based_nett;
		}
	}
	return ret;
}

////////////////////////////////////// CONTROL FLOW ///////////////////////////////////

void decideAction()
{
	if (g_spells.size() < MIN_SPELLS)
	{
		if (hasFreeloader())
		{
			int tome_id = getFreeloader();
			if (g_tomes[tome_id].haveRequiredStones(g_inv))
				cout << "LEARN " << tome_id << endl;
			else
				cout << "LEARN " << getFreeTome() << endl;
		}
		else
			cout << "LEARN " << getFreeTome() << endl;
	}
	else
	{
		searchRecipes();
		cout << g_step << endl;
	}
}

void turnSummary(int &turn)
{
	cerr << "Turn " << turn << " | " << Timer::getTime() << " ms | " << g_nodes_searched << " nodes | " << g_solutions_found << " solutions " << endl;
	turn++;
}

int main()
{
	int turn = 1;

	initializeNodes();
	initializeLogs();

	while (true)
	{
		g_nodes_searched = 0;
		g_solutions_found = 0;
		processInput();
		Timer::setTimer();
		decideAction();
		turnSummary(turn);
	}
}

/////////////////////////////////// INPUT & INITIALIZATIONS ///////////////////////////////////

void processActions()
{
	int action_count;
	cin >> action_count;
	cin.ignore();

	for (int i = 0; i < action_count; i++)
	{
		int id;			 // the unique ID of this spell or recipe
		string type;	 // CAST, OPPONENT_CAST, LEARN, BREW
		int blue;		 // tier-0 ingredient change
		int green;		 // tier-1 ingredient change
		int orange;		 // tier-2 ingredient change
		int yellow;		 // tier-3 ingredient change
		int price;		 // the price in rupees if this is a potion
		bool castable;	 // 1 if this is a castable player spell
		int tome_index;	 // the index in the tome if this is a tome spell, equal to the read-ahead tax
		int tax_count;	 // the amount of taxed tier-0 ingredients you gain from learning this spell
		bool repeatable; // 1 if this is a repeatable player spell

		cin >> id >> type >> blue >> green >> orange >> yellow >> price >> tome_index >> tax_count >> castable >> repeatable;
		cin.ignore();

		if (type == "BREW")
			g_recipes.insert({id, Recipe(blue, green, orange, yellow, id, price)});
		else if (type == "CAST")
			g_spells.insert({id, Spell(blue, green, orange, yellow, id, castable, repeatable)});
		else if (type == "LEARN")
			g_tomes.insert({id, Tome(blue, green, orange, yellow, id, tome_index, tax_count, repeatable)});
	}
}

void processInventory()
{
	for (int i = 0; i < 2; i++)
	{
		int blue;
		int green;
		int orange;
		int yellow;
		int score;

		cin >> blue >> green >> orange >> yellow >> score;
		cin.ignore();

		if (i == 0)
			g_inv = Inventory(blue, green, orange, yellow, score);
	}
}

void processInput()
{
	g_recipes.clear();
	g_spells.clear();
	g_tomes.clear();

	processActions();
	processInventory();
}