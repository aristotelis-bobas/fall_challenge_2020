
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

#define DEPTH_LIMIT 8
#define PRUNING_DEPTH 6

#define EXPLORATION_DEPTH_LIMIT 12
#define EXPLORATION_PRUNING_DEPTH 6

#define MAX_SPELLS 11
#define MAX_TAX_COST 1
#define TIMER 45

#define REST 777
#define NODE_COUNT 50000
#define LOG_COUNT 1000

#define TIER_1_VALUE 1
#define TIER_2_VALUE 2
#define TIER_3_VALUE 3
#define TIER_4_VALUE 4

using namespace std;
using namespace std::chrono;

void processInput();
int getFreeTome();
void addLog(vector<int> log);
int getDepthLimit();
class Action;

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

class Inventory : public Stones
{
public:
	int score;
	int slots_filled;

	Inventory() {}
	Inventory(int blue, int green, int orange, int yellow, int score)
		: Stones(blue, green, orange, yellow), score(score)
	{
		slots_filled = stones[0] + stones[1] + stones[2] + stones[3];
	}
	Inventory(const Inventory &rhs)
	{
		slots_filled = rhs.slots_filled;
		for (int i = 0; i < 4; i++)
			stones[i] = rhs.stones[i];
	}
	Inventory &operator=(const Inventory &rhs)
	{
		slots_filled = rhs.slots_filled;
		for (int i = 0; i < 4; i++)
			stones[i] = rhs.stones[i];
		return *this;
	}

	template <typename Action>
	void mutateInventory(const Action &action, int mult = 1)
	{
		for (int i = 0; i < 4; i++)
		{
			if (action.stones[i] < 0)
				stones[i] -= mult * abs(action.stones[i]);
			if (action.stones[i] > 0)
				stones[i] += mult * action.stones[i];
		}
		slots_filled = 0;
		for (int i = 0; i < 4; i++)
		{
			if (stones[i] > 0)
				slots_filled += stones[i];
		}
	}
};

bool operator==(const Inventory &lhs, const Inventory &rhs)
{
	for (int i = 0; i < 4; i++)
	{
		if (lhs.stones[i] != rhs.stones[i])
			return false;
	}
	return true;
}

bool operator!=(const Inventory &lhs, const Inventory &rhs)
{
	if (lhs == rhs)
		return false;
	return true;
}

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
	bool special = false;

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
		{
			freeloader = true;
			if (stones[2] > 0 || stones[3] > 0)
				special = true;
		}
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
vector<vector<vector<int>>> g_logs;
map<int, int> g_depths;
map<int, int> g_log_pair;
map<int, Recipe> g_recipes;
map<int, Tome> g_tomes;
map<int, Spell> g_spells;
Inventory g_inv;
string g_step;
int enemy_score;
int g_nodes_searched;
int g_solutions_found;
int g_potions_brewed;
int g_turn;

bool g_early_exploration;
bool g_pre_completion_exploration;
bool g_post_completion_exploration = false;

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
	multimap<int, int> repeats;
	bool rest_option = false;

	/// class state
	static map<int, vector<int>> targets;
	static int limit;
	static int prune_depth;
	static int nodes_searched;
	static int target_hits;
	static int limit_hits;

	// root_node constructor
	Node(Inventory start_inv, map<int, Spell> search_spells, map<int, vector<int>> search_targets, int depth_limit, int prune)
	{
		// initialize class state
		Node::targets = search_targets;
		Node::limit = depth_limit;
		Node::nodes_searched = 0;
		Node::target_hits = 0;
		Node::prune_depth = prune;

		// initialize object state
		inv = {start_inv.stones[0], start_inv.stones[1], start_inv.stones[2], start_inv.stones[3], start_inv.slots_filled};
		for (auto &spell : search_spells)
			spells[spell.first] = spell.second.avail;
		depth = 0;

		targetHit();
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
	void createChildNode(array<int, 5> &&mutated_inv, map<int, bool> &&mutated_spells, vector<int> &&mutated_log, int mutated_depth)
	{
		cleanNode();

		Node::nodes_searched++;
		inv = mutated_inv;
		spells = mutated_spells;
		log = mutated_log;
		depth = mutated_depth;

		targetHit();
		Timer::checkEndOfTurn();
		if (limitHit())
			return;

		getOptions();
		increaseDepth();
	}

	void increaseDepth()
	{
		for (auto &spell : repeats)
		{
			g_nodes[Node::nodes_searched].createChildNode(mutatedInventory(spell.first, spell.second), mutatedSpells(spell.first), mutatedLog(spell.first * spell.second), depth + 1);
		}
		for (auto &spell_id : options)
		{
			if (depth >= Node::prune_depth)
			{
				if (nodePruned())
					continue;
			}
			g_nodes[Node::nodes_searched].createChildNode(mutatedInventory(spell_id), mutatedSpells(spell_id), mutatedLog(spell_id), depth + 1);
		}
		if (rest_option)
		{
			map<int, bool> mutated_spells = spells;
			mutateSpellAvailability(mutated_spells);
			g_nodes[Node::nodes_searched].createChildNode(std::move(inv), std::move(mutated_spells), mutatedLog(REST), depth + 1);
		}
	}

	void getOptions()
	{
		for (auto it = spells.rbegin(); it != spells.rend(); it++)
		{
			if (g_spells[it->first].haveRequiredStones(inv) && !g_spells[it->first].willOverflowInventory(inv) && it->second)
			{
				options.push_back(it->first);
			}
			if (g_spells[it->first].repeat && it->second)
			{
				for (int mult = 2; mult < 5; mult++)
				{
					if (g_spells[it->first].haveRequiredStones(inv, mult) && !g_spells[it->first].willOverflowInventory(inv, mult))
						repeats.insert({it->first, mult});
					else
					{
						break;
					}
				}
			}
			if (!rest_option && !it->second)
				rest_option = true;
		}
		option_count = options.size() + repeats.size();
		if (rest_option)
			option_count++;
	}

	// eliminating options to search deeper in shorter time
	bool nodePruned()
	{
		if (rand() % 4 == 0)
			return true;
		return false;
	}

	bool limitHit()
	{
		if (depth + 1 > Node::limit)
			return true;
		return false;
	}

	void targetHit()
	{
		int matches;

		for (auto &target : targets)
		{
			matches = 0;
			for (int i = 0; i < 4; i++)
			{
				if (inv[i] < target.second[i])
					break;
				else
					matches++;
			}
			if (matches == 4)
			{
				if (depth <= g_depths[target.first])
				{
					g_depths[target.first] = depth;
					Node::target_hits++;
					vector<int> tmp = log;
					tmp.emplace_back(target.first);
					addLog(tmp);
				}
			}
		}
	}

	array<int, 5> mutatedInventory(int spell_id, int mult = 1)
	{
		array<int, 5> tmp = inv;
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
int Node::prune_depth;

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

/// retrieves all minimum step options and finds the most gain (based on ingredient tier) / step option

vector<int> getOptimalLog(vector<vector<int>> result_logs)
{
	vector<int> result;

	// no solutions
	if (result_logs.empty())
		return result;

	// only 1 solution
	if (result_logs.size() == 1)
		return result_logs[0];

	// filter out suboptimal logs
	vector<vector<int>> logs;

	// finding min amount steps
	int min_steps = 2000;

	for (auto &log : result_logs)
	{
		if (log.size() < min_steps)
			min_steps = log.size();
	}

	for (auto &log : result_logs)
	{
		if (log.size() == min_steps)
			logs.emplace_back(log);
	}

	for (int i = 0; i < min_steps; i++)
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
				if (instruction.first == 0)
				{
					gains.emplace_back(0);
					continue;
				}
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
	g_logs[g_log_pair[log.back()]].emplace_back(log);
}

void pairLogs()
{
	int i = 0;

	g_log_pair.clear();
	for (auto &recipe : g_recipes)
	{
		g_log_pair[recipe.first] = i;
		i++;
	}
}

void setLogsDepth()
{
	g_depths.clear();
	for (auto &recipe : g_recipes)
	{
		g_depths[recipe.first] = getDepthLimit();
	}
}

void initializeLogs()
{
	vector<vector<int>> tmp;

	for (int i = 0; i < 5; i++)
	{
		g_logs.emplace_back(tmp);
		g_logs[i].reserve(LOG_COUNT);
	}
}

void clearLogs()
{
	for (auto &log : g_logs)
	{
		log.clear();
	}
}

////////////////////////////////////// STEPS  //////////////////////////////////////

class Steps
{
public:
	vector<vector<int>> optimal_logs;
	vector<int> output_log;

	float saved_rating;
	vector<int> saved_recipe;
	Inventory saved_inv;

	bool empty;

	Steps() {}

	void checkEmpty()
	{
		for (auto &log : optimal_logs)
		{
			if (!log.empty())
			{
				empty = false;
				return;
			}
		}
		empty = true;
	}

	void processLogs()
	{
		for (auto &recipe_logs : g_logs)
		{
			vector<int> optimal_log = getOptimalLog(recipe_logs);

			if (!optimal_log.empty())
				optimal_logs.emplace_back(optimal_log);
		}

		checkEmpty();
		if (!empty)
			getRating();
	}

	float calculateRating(int price, int steps)
	{
		// if (g_potions_brewed < 2)
		// 	return (float)price / steps;
		// else
		// 	return (float)price / pow(steps, g_potions_brewed);

		return pow(price, 2) / (steps + 1);
	}

	void getRating()
	{
		map<int, float> ratings;
		map<int, vector<int>> recipes;
		float best_rating = -999;
		int best_recipe = 0;

		for (auto recipe : optimal_logs)
		{
			int recipe_id = recipe.back();
			recipes[recipe_id] = recipe;
		}
		cerr << "=======================================" << endl;
		for (auto recipe : optimal_logs)
		{
			int recipe_id = recipe.back();
			ratings[recipe_id] = calculateRating(g_recipes[recipe_id].price, recipe.size());
			cerr << "Recipe " << recipe_id << " | " << ratings[recipe_id] << " | " << g_recipes[recipe_id].price << " gold | " << recipe.size() << " steps" << endl;
		}
		for (auto rating : ratings)
		{
			if (rating.second > best_rating)
			{
				best_rating = rating.second;
				best_recipe = rating.first;
			}
		}

		cerr << "=======================================" << endl;
		if (!saved_recipe.empty())
			cerr << "Saved recipe " << saved_recipe.back() << " | " << saved_rating << " | " << g_recipes[saved_recipe.back()].price << " gold | " << saved_recipe.size() << " steps" << endl;

		output_log = recipes[best_recipe];
		checkSavedSteps(best_recipe, best_rating);
		saveBestSteps();
		cerr << "Target recipe " << best_recipe << " | " << best_rating << " | " << g_recipes[best_recipe].price << " gold | " << output_log.size() << " steps" << endl;
		cerr << "=======================================" << endl;
	}

	void setOutput()
	{
		if (empty)
		{
			getFreeStep();
			return;
		}
		if (output_log.size() == 1)
		{
			for (auto &recipe : g_recipes)
			{
				if (recipe.second.id == output_log.front())
				{
					g_potions_brewed++;
					// g_post_completion_exploration = true;
					// cerr << "Post completion exploration enabled" << endl;
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
		else if (output_log.front() == REST)
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

	void getFreeStep()
	{
		for (auto spell : g_spells)
		{
			if (spell.second.haveRequiredStones(g_inv) && spell.second.avail && !spell.second.willOverflowInventory(g_inv))
			{
				g_step = "CAST " + to_string(spell.first);
				return;
			}
		}
		g_step = "REST";
	}

	void checkSavedSteps(int &best_recipe, float &best_rating)
	{
		if (g_pre_completion_exploration)
		{
			// save mutated inventory state before wipe
			Inventory tmp = g_inv;
			tmp.mutateInventory(g_recipes[saved_recipe.front()]);
			clearSaved();
			g_inv = tmp;
			return;
		}
		if (!saved_recipe.empty())
		{
			if (g_inv != saved_inv)
			{
				cerr << "Inventory has changed; discarded saved steps" << endl;
				clearSaved();
				return;
			}
			if (best_rating < saved_rating)
			{
				output_log = saved_recipe;
				best_recipe = output_log.back();
				best_rating = saved_rating;
			}
		}
	}

	void saveBestSteps()
	{
		if (output_log.size() > 1)
		{
			// saving steps

			if (!g_early_exploration && !g_pre_completion_exploration)
				saved_recipe = vector<int>(output_log.begin() + 1, output_log.end());
			else
				saved_recipe = output_log;

			// saving inventory state

			if (g_early_exploration)
				saved_inv = g_inv;
			else if (!g_pre_completion_exploration)
			{
				saved_inv = g_inv;
				if (output_log.front() != REST)
				{
					if (isRepeatingSpell(output_log.front()))
					{
						pair<int, int> instruction = getRepeatingSpell(output_log.front());
						if (instruction.first != 0)
							saved_inv.mutateInventory(g_spells[instruction.first], instruction.second);
					}
					else
						saved_inv.mutateInventory(g_spells[output_log.front()]);
				}
			}

			// saving rating
			saved_rating = calculateRating(g_recipes[saved_recipe.back()].price, saved_recipe.size());
		}
		else
			clearSaved();
	}

	void checkSavedRecipeExist()
	{
		for (auto recipe : g_recipes)
		{
			if (recipe.second.id == saved_recipe.back())
				return;
		}
		cerr << "Saved recipe brewed by opponent" << endl;
		clearSaved();
	}

	void clearLogs()
	{
		optimal_logs.clear();
	}

	void clearSaved()
	{
		saved_recipe.clear();
		saved_rating = 0;
		saved_inv = Inventory();
	}
};

Steps g_steps;

//////////////////////////////////////////////// SEARCHES /////////////////////////////////////////

int getDepthLimit()
{
	if (g_early_exploration || g_pre_completion_exploration || g_post_completion_exploration)
		return EXPLORATION_DEPTH_LIMIT;
	else
		return DEPTH_LIMIT;
}

int getPruneDepth()
{
	if (g_early_exploration || g_pre_completion_exploration || g_post_completion_exploration)
		return EXPLORATION_PRUNING_DEPTH;
	else
		return PRUNING_DEPTH;
}

bool earlyExploration()
{
	if (g_spells.size() < MAX_SPELLS - 4)
	{
		g_early_exploration = false;
		return false;
	}
	if (g_spells.size() >= MAX_SPELLS - 4 && g_spells.size() < MAX_SPELLS)
	{
		cerr << "Early exploration enabled" << endl;
		g_early_exploration = true;
	}
	else
		g_early_exploration = false;
	return true;
}

void checkCompletion()
{
	if (g_steps.saved_recipe.size() != 1)
	{
		if (g_pre_completion_exploration)
			cerr << "Pre completion exploration disabled" << endl;
		g_pre_completion_exploration = false;
		return;
	}
	cerr << "Pre completion exploration enabled" << endl;
	g_pre_completion_exploration = true;
	g_potions_brewed++;
	g_step = "BREW " + to_string(g_steps.saved_recipe.front());
}

Inventory getInventory()
{
	if (!g_pre_completion_exploration)
		return g_inv;

	Inventory tmp = g_inv;
	int recipe_id = g_steps.saved_recipe.front();
	tmp.mutateInventory(g_recipes[recipe_id]);

	return tmp;
}

void explorationLogic()
{
	// // disable post completion exploration after 1 cycle
	// if (g_post_completion_exploration)
	// {
	// 	cerr << "Post completion exploration disabled" << endl;
	// 	g_post_completion_exploration = false;
	// }

	// main output set elsewhere
	if (!g_early_exploration && !g_pre_completion_exploration)
		g_steps.setOutput();

	// // enable post completion exploration here when saved recipe is brewed, otherwise in setOutput()
	// if (g_pre_completion_exploration)
	// {
	// 	cerr << "Post completion exploration enabled" << endl;
	// 	g_post_completion_exploration = true;
	// }
}

void launchSearch()
{
	map<int, vector<int>> search_targets;

	if (!earlyExploration())
	{
		cerr << "Not yet time to search." << endl;
		return;
	}

	if (!g_steps.saved_recipe.empty())
		g_steps.checkSavedRecipeExist();

	if (!g_steps.saved_recipe.empty())
		checkCompletion();

	pairLogs();
	setLogsDepth();

	for (auto &recipe : g_recipes)
	{
		if (g_pre_completion_exploration)
		{
			if (recipe.first == g_steps.saved_recipe.front())
				continue;
		}
		search_targets[recipe.first] = recipe.second.getCost();
	}

	try
	{
		Node root_node(getInventory(), g_spells, search_targets, getDepthLimit(), getPruneDepth());
	}
	catch (char const *e)
	{
		g_solutions_found += Node::target_hits;
		g_nodes_searched += Node::nodes_searched;
	}

	g_steps.processLogs();
	explorationLogic();
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
		if (tome.second.freeloader && tome.second.tax_cost <= MAX_TAX_COST)
			return true;
	}
	return false;
}

bool hasSpecial()
{
	for (auto tome : g_tomes)
	{
		if (tome.second.special && tome.second.tax_cost <= MAX_TAX_COST)
			return true;
	}
	return false;
}

int getSpecial()
{
	for (auto tome : g_tomes)
	{
		if (tome.second.special && tome.second.tax_cost <= MAX_TAX_COST)
			return tome.first;
	}
	return -1;
}

int getFreeloader()
{
	for (auto tome : g_tomes)
	{
		if (tome.second.freeloader && tome.second.tax_cost <= MAX_TAX_COST)
			return tome.first;
	}
	return -1;
}

////////////////////////////////////// CONTROL FLOW ///////////////////////////////////

void turnAction()
{
	launchSearch();
	if (g_spells.size() < MAX_SPELLS || hasSpecial() && g_potions_brewed < 3)
	{
		if (hasSpecial())
		{
			int tome_id = getSpecial();
			if (g_tomes[tome_id].haveRequiredStones(g_inv))
			{
				if (g_spells.size() >= MAX_SPELLS)
					g_steps.clearSaved();
				cout << "LEARN " << tome_id << endl;
				return;
			}
		}
		if (hasFreeloader())
		{
			int tome_id = getFreeloader();
			if (g_tomes[tome_id].haveRequiredStones(g_inv))
			{
				cout << "LEARN " << tome_id << endl;
				return;
			}
		}
		cout << "LEARN " << getFreeTome() << endl;
		return;
	}
	cout << g_step << endl;
}

////////////////////////////////////// MAIN ///////////////////////////////////

void initializationsOnce()
{
	srand(time(0));
	initializeNodes();
	initializeLogs();

	g_potions_brewed = 0;
	g_turn = 1;
}

void turnResets()
{
	g_recipes.clear();
	g_spells.clear();
	g_tomes.clear();
	g_solutions_found = 0;
	g_nodes_searched = 0;
	g_steps.clearLogs();
	clearLogs();
}

void testStuff()
{
	// testInput();
	// benchmarkCycles(50);
	// benchmarkAllRecipes();
	// printData();
}

void turnSummary()
{
	cerr << "Turn " << g_turn << " | " << Timer::getTime() << " ms | " << g_nodes_searched << " nodes | " << g_solutions_found << " solutions " << endl;
	g_turn++;
}

int main()
{
	initializationsOnce();

	while (true)
	{
		turnResets();
		processInput();
		Timer::setTimer();
		turnAction();
		turnSummary();
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
		if (i == 1)
			enemy_score = score;
	}
}

void processInput()
{
	processActions();
	processInventory();
}