
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
#include <cstdlib>

#define DEPTH_LIMIT 12
#define SURVEY_DEPTH 3

#define MAX_SPELLS 13
#define TIMER_CUTOFF 47

#define NODE_REST 777
#define NODE_COUNT 60000
#define LOG_COUNT 10000

#define TIER_1_VALUE 1
#define TIER_2_VALUE 2
#define TIER_3_VALUE 3
#define TIER_4_VALUE 4

using namespace std;
using namespace std::chrono;

void processInput();
int getFreeTome();
void printOptimalLog(vector<int> result);
void printLogs(vector<vector<int>> results);
void testInput();
void benchmarkCycles(int cycles);
void benchmarkAllRecipes();

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
	double getResult() { return time_span.count(); }
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
		if (Timer::time_span.count() >= TIMER_CUTOFF)
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
string g_step;
int enemy_score;
int g_nodes_searched;
int g_solutions_found;
int g_potions_brewed;
int g_prune_depth;
int g_turn;
bool g_survey = true;
bool g_pure_survey = true;

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
			if (log_id / i == spell.second.id)
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
	map<int, int> repeats;
	bool rest_option = false;

	/// class state
	static map<int, vector<int>> targets;
	static int limit;
	static int depth_cutoff;
	static int nodes_searched;
	static int target_hits;
	static int limit_hits;

	// root_node constructor
	Node(Inventory start_inv, map<int, Spell> search_spells, map<int, vector<int>> search_targets, int cutoff)
	{
		// initialize class state
		Node::targets = search_targets;
		Node::limit = DEPTH_LIMIT;
		Node::nodes_searched = 0;
		Node::target_hits = 0;
		Node::limit_hits = 0;
		Node::depth_cutoff = cutoff;

		// initialize object state
		inv = {start_inv.stones[0], start_inv.stones[1], start_inv.stones[2], start_inv.stones[3], start_inv.slots_filled};
		for (auto &spell : search_spells)
			spells[spell.first] = spell.second.avail;
		depth = 0;

		if (targetHit())
			return;
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
		if (limitHit())
			return;
		Timer::checkEndOfTurn();

		// expansion mechanics
		getOptions();
		increaseDepth();
	}

	void increaseDepth()
	{
		for (auto spell_id : options)
		{
			if (g_survey && depth >= Node::depth_cutoff)
			{
				if (nodePruned())
					continue;
			}
			g_nodes[Node::nodes_searched].createChildNode(mutatedInventory(spell_id), mutatedSpells(spell_id), mutatedLog(spell_id), depth + 1);
		}
		for (auto spell : repeats)
		{
			g_nodes[Node::nodes_searched].createChildNode(mutatedInventory(spell.first, spell.second), mutatedSpells(spell.first), mutatedLog(spell.first * spell.second), depth + 1);
		}
		if (rest_option)
		{
			map<int, bool> mutated_spells = spells;
			mutateSpellAvailability(mutated_spells);

			g_nodes[Node::nodes_searched].createChildNode(inv, mutated_spells, mutatedLog(NODE_REST), depth + 1);
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

	// eliminating options to search deeper in shorter time
	bool nodePruned()
	{
		if (rand() % 3 == 0)
			return false;
		return true;
	}

	// turn based or game limits //
	bool capacityHit()
	{
		if (Node::nodes_searched + 1 > NODE_COUNT)
			return true;
		if (Node::target_hits + 1 > LOG_COUNT)
			return true;
		return false;
	}

	bool limitHit()
	{
		if (depth + 1 > Node::limit)
		{
			Node::limit_hits++;
			return true;
		}
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
int Node::limit_hits;
int Node::depth_cutoff;

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

////////////////////////////////////// LOGS  //////////////////////////////////////

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

	// // debugging
	// printLogs(logs);

	for (int i = 0; i < Node::limit; i++)
	{
		// getting tier based gain of log current step

		vector<int> gains;

		for (int y = 0; y < logs.size(); y++)
		{
			if (logs[y][i] == NODE_REST)
				gains.emplace_back(0);
			else if (isRepeatingSpell(logs[y][i]))
			{
				pair<int, int> instruction = getRepeatingSpell(logs[y][i]);
				gains.emplace_back(g_spells[instruction.first].tier_based_nett * instruction.second);
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

	// // debugging
	// printOptimalLog(logs.front());

	return logs.front();
}

void addLog(vector<int> log)
{
	g_logs.emplace_back(log);
}

void initializeLogs()
{
	g_logs.reserve(LOG_COUNT);
}

////////////////////////////////////// STEPS  //////////////////////////////////////

class Steps
{
public:
	vector<vector<int>> optimal_logs;
	vector<int> output_log;

	float saved_rating;
	vector<int> saved_recipe;

	Steps() {}

	void getStep()
	{
		bool found_solution = false;

		for (auto log : optimal_logs)
		{
			if (!log.empty())
			{
				found_solution = true;
				break;
			}
		}
		if (!found_solution)
		{
			getFreeStep();
			return;
		}

		// 2x greedy - 4x fast
		// if (g_potions_brewed >= 2)
		// {
		// 	fastStrategy();
		// 	return;
		// }
		// greedyStrategy();

		// 5x greedy - 1x fast
		if (g_potions_brewed >= 5)
		{
			fastStrategy();
			return;
		}
		greedyStrategy();

		// 3x power - 3x fast
		// if (g_potions_brewed >= 3)
		// {
		// 	fastStrategy();
		// 	return;
		// }
		// powerStrategy();

		// 3x power - 3x greedy
		// if (g_potions_brewed >= 3)
		// {
		// 	greedyStrategy();
		// 	return;
		// }
		// powerStrategy();

		// 6x power
		// powerStrategy();

		// 6x greedy
		// greedyStrategy();
	}

	void setOutput()
	{
		if (output_log.size() == 1)
		{
			for (auto &recipe : g_recipes)
			{
				if (recipe.second.id == output_log.front())
				{
					g_potions_brewed++;
					if (g_potions_brewed < 5)
					{
						g_survey = true;
						cerr << "Survey mode enabled." << endl;
					}
					g_step = "BREW " + to_string(output_log.front());
					return;
				}
			}
			for (auto &tome : g_tomes)
			{
				if (tome.second.id == output_log.front())
				{
					g_step = "LEARN " + to_string(output_log.front());
					return;
				}
			}
		}
		else if (output_log.front() == NODE_REST)
		{
			g_step = "REST";
			return;
		}
		else if (isRepeatingSpell(output_log.front()))
		{
			pair<int, int> repeat = getRepeatingSpell(output_log.front());
			g_step = "CAST " + to_string(repeat.first) + " " + to_string(repeat.second);
			return;
		}
		else
		{
			g_step = "CAST " + to_string(output_log.front());
		}
	}

	void fastStrategy()
	{
		vector<vector<int>> quickest_logs;
		int smallest = 2000;

		for (auto &log : optimal_logs)
		{
			if (log.size() < smallest)
				smallest = log.size();
		}
		for (auto log : optimal_logs)
		{
			if (log.size() == smallest)
				quickest_logs.emplace_back(log);
		}

		optimal_logs = quickest_logs;
		greedyStrategy();
	}

	void powerStrategy()
	{
		map<int, float> ratings;
		map<int, vector<int>> recipes;

		for (auto recipe : optimal_logs)
		{
			int recipe_id = recipe.back();
			recipes[recipe_id] = recipe;
		}

		for (auto recipe : optimal_logs)
		{
			int recipe_id = recipe.back();
			ratings[recipe_id] = pow(g_recipes[recipe_id].price, 2) / (recipe.size() + 1);
			// cerr << "Recipe [power] " << recipe_id << " | " << g_recipes[recipe_id].price << " / " << recipe.size() << " = " << ratings[recipe_id] << "." << endl;
		}

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
		output_log = recipes[best_recipe];

		// check whether saved recipe is still in game

		if (!saved_recipe.empty())
			checkSavedRecipeExist();

		//debugging
		// cerr << "Saved best_rating: " << saved_rating << endl;
		// cerr << "Found best_rating: " << best_rating << endl;

		if (!saved_recipe.empty())
		{
			if (best_rating <= saved_rating)
			{
				output_log = saved_recipe;
				best_recipe = output_log.back();
				best_rating = saved_rating;
			}
		}

		if (output_log.size() > 1)
		{
			if (!g_pure_survey)
				saved_recipe = vector<int>(output_log.begin() + 1, output_log.end());
			else
				saved_recipe = output_log;
			saved_rating = pow(g_recipes[saved_recipe.back()].price, 2) / (saved_recipe.size() + 1);
		}
		else
			clearSaved();

		// debugging
		// cerr << "__________________________________" << endl;
		cerr << "Target recipe " << best_recipe << " | " << g_recipes[best_recipe].price << " gold / " << output_log.size() << " steps = " << best_rating << " rating" << endl;
		//printOptimalLog(output_log);

		setOutput();
	}

	void greedyStrategy()
	{
		map<int, float> ratings;
		map<int, vector<int>> recipes;

		for (auto recipe : optimal_logs)
		{
			int recipe_id = recipe.back();
			recipes[recipe_id] = recipe;
		}

		for (auto recipe : optimal_logs)
		{
			int recipe_id = recipe.back();
			ratings[recipe_id] = (float)g_recipes[recipe_id].price / (recipe.size() + 1);
			// cerr << "Recipe [greedy] " << recipe_id << " | " << g_recipes[recipe_id].price << " / " << recipe.size() << " = " << ratings[recipe_id] << "." << endl;
		}

		float best_rating = -999;
		int best_recipe = 0;
		for (auto rating : ratings)
		{
			if (rating.second >= best_rating)
			{
				if (rating.second == best_rating)
				{
					if (recipes[rating.first].size() > recipes[best_recipe].size())
						continue;
				}
				best_rating = rating.second;
				best_recipe = rating.first;
			}
		}
		output_log = recipes[best_recipe];

		// check whether saved recipe is still in game

		if (!saved_recipe.empty())
			checkSavedRecipeExist();

		// debugging
		// cerr << "Saved best_rating: " << saved_rating << endl;
		// cerr << "Found best_rating: " << best_rating << endl;

		if (!saved_recipe.empty())
		{
			if (best_rating <= saved_rating)
			{
				output_log = saved_recipe;
				best_recipe = output_log.back();
				best_rating = saved_rating;
			}
		}

		if (output_log.size() > 1)
		{
			if (!g_pure_survey)
				saved_recipe = vector<int>(output_log.begin() + 1, output_log.end());
			else
				saved_recipe = output_log;
			saved_rating = (float)g_recipes[saved_recipe.back()].price / (saved_recipe.size() + 1);
		}
		else
			clearSaved();

		// debugging
		// cerr << "__________________________________" << endl;
		cerr << "Target " << best_recipe << " | " << g_recipes[best_recipe].price << " / " << output_log.size() << " = " << best_rating << "." << endl;

		setOutput();
	}

	void getFreeStep()
	{
		for (auto &spell : g_spells)
		{
			if (spell.second.haveRequiredStones(g_inv) && spell.second.avail && !spell.second.willOverflowInventory(g_inv))
			{
				g_step = "CAST " + to_string(spell.first);
				return;
			}
		}
		g_step = "LEARN " + to_string(getFreeTome());
	}

	void checkSavedRecipeExist()
	{
		for (auto recipe : g_recipes)
		{
			if (recipe.second.id == saved_recipe.back())
				return;
		}
		g_survey = true;
		cerr << "Survey mode enabled" << endl;
		clearSaved();
	}

	void clearLogs() { optimal_logs.clear(); }

	void addOptimalLog(vector<int> optimal_log) { optimal_logs.emplace_back(optimal_log); }

	void clearSaved()
	{
		saved_recipe.clear();
		saved_rating = 0;
	}
};

Steps g_steps;

//////////////////////////////////////////////// SEARCHES /////////////////////////////////////////

void searchRecipes()
{
	map<int, vector<int>> search_targets;
	vector<int> recipes_searched;
	vector<int> optimal_log;

	// benchmarking search
	Benchmark bm;

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
		bm.startBenchmark();
		try
		{
			Node root_node(g_inv, g_spells, search_targets, g_prune_depth);

			// benchmark results
			bm.endBenchmark();
			cerr << Node::nodes_searched << " nodes | " << bm.getResult() << " ms | ";
			cerr << Node::target_hits << " hits" << endl;

			optimal_log = getOptimalLog();
			if (!optimal_log.empty())
			{
				g_steps.addOptimalLog(optimal_log);
				recipes_searched.emplace_back(optimal_log.back());
			}
		}
		catch (char const *e)
		{
			// benchmark results
			bm.endBenchmark();
			cerr << Node::nodes_searched << " nodes | " << bm.getResult() << " ms | ";
			cerr << Node::target_hits << " hits" << endl;

			g_solutions_found += Node::target_hits;
			g_nodes_searched += Node::nodes_searched;
			optimal_log = getOptimalLog();
			if (!optimal_log.empty())
			{
				g_steps.addOptimalLog(optimal_log);
				recipes_searched.emplace_back(optimal_log.back());
			}
			break;
		}
	}
	g_steps.getStep();
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
		if (tome.second.freeloader && tome.second.tax_cost < 2)
			return true;
	}
	return false;
}

int getOptimalFreeloader()
{
	int max_gain = -999;
	vector<int> tmp_tomes;
	int ret = 0;

	for (auto tome : g_tomes)
	{
		if (tome.second.freeloader && tome.second.tax_cost < 2)
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

////////////////////////////////////// HIGH LEVEL CONTROL FLOW ///////////////////////////////////

void decideAction()
{
	if (g_spells.size() < MAX_SPELLS)
	{
		// if (hasFreeloader())
		// {
		// 	int tome_id = getOptimalFreeloader();
		// 	if (g_tomes[tome_id].haveRequiredStones(g_inv))
		// 		cout << "LEARN " << tome_id << endl;
		// 	else
		// 		cout << "LEARN " << getFreeTome() << endl;
		// }
		// else
		// {
		cout << "LEARN " << getFreeTome() << endl;
		// if (g_turn >= 7)
		// 	searchRecipes();
	}
	else
	{
		if (g_pure_survey)
			g_pure_survey = false;
		searchRecipes();
		cout << g_step << endl;
	}
}

int main()
{
	initializeNodes();
	initializeLogs();

	g_potions_brewed = 0;
	g_prune_depth = SURVEY_DEPTH;
	g_turn = 1;

	srand(time(0));

	int counter = 0;

	while (true)
	{
		// cleaning data
		g_solutions_found = 0;
		g_nodes_searched = 0;
		g_steps.clearLogs();

		// /// benchmarking ///
		// testInput();
		// // //benchmarkCycles(50);
		// benchmarkAllRecipes();

		/// debugging ///
		//printData();

		/// live input ///
		processInput();

		// disable survey mode
		if (g_spells.size() >= MAX_SPELLS && g_survey)
		{
			if (counter == 2)
			{
				g_survey = false;
				cerr << "Survey mode disabled" << endl;
				counter = 0;
			}
			else if (counter < 2)
				counter++;
		}

		Timer::setTimer();
		decideAction();

		cerr << "Turn " << g_turn << " | " << Timer::getTime() << " ms | " << g_nodes_searched << " nodes | " << g_solutions_found << " solutions " << endl;

		g_turn++;
	}
}

//////////////////////////////////////////////// BENCHMARK /////////////////////////////////////////

// void benchmarkAllRecipes()
// {
// 	map<int, vector<int>> search_targets;
// 	vector<vector<int>> optimal_logs;
// 	vector<int> recipes_searched;
// 	vector<int> optimal_log;

// 	Benchmark bm;
// 	bm.startBenchmark();
// 	g_timer = Timer();

// 	while (!g_timer.endOfTurn())
// 	{
// 		search_targets.clear();
// 		g_logs.clear();

// 		for (auto recipe : g_recipes)
// 		{
// 			bool already_searched = false;
// 			for (auto searched : recipes_searched)
// 			{
// 				if (recipe.first == searched)
// 				{
// 					already_searched = true;
// 					break;
// 				}
// 			}
// 			if (!already_searched)
// 				search_targets[recipe.first] = recipe.second.getCost();
// 		}
// 		if (search_targets.empty())
// 			break;
// 		Node root_node(g_inv, g_spells, search_targets);
// 		// cerr << "exited at " << g_timer.getResult() << " ms." << endl;
// 		optimal_log = getOptimalLog();
// 		if (optimal_log.empty())
// 			break;
// 		optimal_logs.emplace_back(optimal_log);
// 		recipes_searched.emplace_back(optimal_log.back());
// 	}
// 	// bm.endBenchmark();
// 	// bm.printResult();
// 	getStep(optimal_logs);
// }

// void benchmarkCycles(int cycles)
// {
// 	map<int, vector<int>> search_targets;
// 	for (auto recipe : g_recipes)
// 		search_targets[recipe.first] = recipe.second.getCost();

// 	Benchmark bm;
// 	bm.startBenchmark();

// 	for (int i = 0; i < cycles; i++)
// 	{
// 		Node root_node(g_inv, g_spells, search_targets);
// 	}
// 	bm.endBenchmark();
// 	cout << "Search time (mean): " << bm.getResult() / cycles << endl;
// }

//////////////////////////////////////////////// DEBUG /////////////////////////////////////////

void printOptimalLog(vector<int> result)
{
	if (result.empty())
	{
		cerr << "No results." << endl;
		return;
	}
	if (result.size() == 1)
	{
		cerr << "All ingredients for action " << result.back() << " are available." << endl;
		return;
	}
	cerr << "Action " << result.back() << " in " << result.size() - 1 << " step(s): ";
	for (auto step : result)
	{
		if (step == result.back())
			continue;
		if (step != NODE_REST)
			cerr << step;
		else
			cerr << "REST";
		cerr << ", ";
	}
	cerr << endl;
}

void printLogs(vector<vector<int>> results)
{
	for (auto log : results)
	{
		cerr << "Action " << log.back() << " in " << log.size() - 1 << " step(s): ";
		for (auto step : log)
		{
			if (step == log.back())
				continue;
			if (step != NODE_REST)
				cerr << step;
			else
				cerr << "REST";
			cerr << ", ";
		}
		cerr << endl;
	}
}

void printSpells()
{
	cerr << "-------------SPELLS-------------" << endl;
	for (auto spell : g_spells)
	{
		cerr << "spell " << spell.first << " [";
		for (int i = 0; i < 4; i++)
		{
			cerr << spell.second.stones[i];
			(i == 3) ? cerr << "]" << endl : cerr << ", ";
		}
	}
}

void printTomes()
{
	cerr << "-------------TOMES-------------" << endl;
	for (auto tome : g_tomes)
	{
		cerr << "tome " << tome.first << " [";
		for (int i = 0; i < 4; i++)
		{
			cerr << tome.second.stones[i];
			(i == 3) ? cerr << "]" : cerr << ", ";
		}
		if (tome.second.freeloader)
			cerr << " - freeloader";
		cerr << endl;
	}
}

void printRecipes()
{
	cerr << "-------------RECIPES-------------" << endl;
	for (auto recipe : g_recipes)
	{
		cerr << "recipe " << recipe.first << " [";
		for (int i = 0; i < 4; i++)
		{
			cerr << recipe.second.stones[i];
			(i == 3) ? cerr << "]" : cerr << ", ";
		}
		cerr << " - rating " << recipe.second.rating << " - steps " << recipe.second.steps << " - " << recipe.second.price << " rupees" << endl;
	}
}

void printData()
{
	printRecipes();
	//printTomes();
	printSpells();
}

/////////////////////////////////// TEST INPUT ///////////////////////////////////

void testInput()
{
	g_recipes.insert({60, Recipe(0, 0, -5, 0, 60, 16)});
	g_recipes.insert({62, Recipe(0, -2, 0, -3, 62, 19)});
	g_recipes.insert({72, Recipe(0, -2, -2, -2, 72, 19)});
	g_recipes.insert({74, Recipe(-3, -1, -1, -1, 74, 14)});
	g_recipes.insert({75, Recipe(-1, -3, -1, -1, 75, 16)});

	g_spells.insert({78, Spell(2, 0, 0, 0, 78, true, false)});
	g_spells.insert({79, Spell(-1, 1, 0, 0, 79, true, false)});
	g_spells.insert({80, Spell(0, -1, 1, 0, 80, true, false)});
	g_spells.insert({81, Spell(0, 0, -1, 1, 81, true, false)});
	g_spells.insert({86, Spell(1, 1, 0, 0, 86, true, false)});
	g_spells.insert({88, Spell(3, -2, 1, 0, 88, true, true)});
	g_spells.insert({90, Spell(4, 0, 0, 0, 90, true, false)});
	g_spells.insert({92, Spell(2, 3, -2, 0, 92, true, true)});
	g_spells.insert({94, Spell(0, 2, -1, 0, 94, true, true)});
	g_spells.insert({96, Spell(-4, 0, 2, 0, 96, true, true)});
	g_spells.insert({98, Spell(0, -3, 0, 2, 98, true, true)});
	g_spells.insert({100, Spell(2, 1, -2, 1, 100, true, true)});
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
		if (i == 1)
			enemy_score = score;
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