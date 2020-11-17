

/// OPTIMIZATIONS /// thnx Stan lol

#pragma GCC optimize("O3")
#pragma GCC optimize("inline")
#pragma GCC optimize("omit-frame-pointer")
#pragma GCC optimize("unroll-loops")
#pragma GCC optimize("Ofast")
#pragma GCC option("arch=native", "tune=native", "no-zero-upper")
#pragma GCC target("avx")
#pragma GCC target "bmi2"
#include <x86intrin.h>

/////////////////////////////////////

#include <iostream>
#include <string>
#include <map>
#include <list>
#include <vector>
#include <chrono>
#include <cmath>

#define NODE_DEPTH_LIMIT 10
#define NODE_REST 777

#define MUTATOR_COST_DEPTH 2
#define MUTATOR_COST_SPREAD 1
#define MUTATOR_GAIN_SPREAD 1

using namespace std;
using namespace std::chrono;

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

	virtual vector<int> getMissingStones(Inventory &inv)
	{
		vector<int> missing(4, 0);

		for (int i = 0; i < 4; i++)
			missing[i] = (stones[i] < 0 && inv.stones[i] < abs(stones[i])) ? abs(stones[i]) - inv.stones[i] : 0;

		return missing;
	}

	virtual bool haveRequiredStones(Inventory &inv)
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
};

class Tome : public Action
{
public:
	int tax_cost;
	int tax_gain;
	int gain_spread;
	int cost_spread;
	int gain_depth;
	int cost_depth;
	bool repeat;
	bool freeloader = false;
	bool mutator = false;

	Tome() {}
	Tome(int blue, int green, int orange, int yellow, int id, int tax_cost, int tax_gain, bool repeat)
		: Action(blue, green, orange, yellow, id), tax_cost(tax_cost), tax_gain(tax_gain), repeat(repeat)
	{
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

		if (cost_spread <= MUTATOR_COST_SPREAD && gain_spread <= MUTATOR_GAIN_SPREAD && cost_depth <= MUTATOR_COST_DEPTH && repeat)
			mutator = true;
	}

	vector<int> getMissingStones(Inventory &inv)
	{
		vector<int> missing(4, 0);

		missing[0] = (inv.stones[0] < tax_cost) ? tax_cost - inv.stones[0] : 0;
		return missing;
	}

	bool haveRequiredStones(Inventory &inv)
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

	Spell() {}
	Spell(int blue, int green, int orange, int yellow, int id, bool avail, bool repeat)
		: Action(blue, green, orange, yellow, id), avail(avail), repeat(repeat) {}

	bool willOverflowInventory(Inventory inv, int mult = 1)
	{
		int count = 0;

		for (int i = 0; i < 4; i++)
			count += (stones[i] * mult);

		if (inv.slots_filled + count > 10)
			return true;

		return false;
	}

	bool haveRequiredStones(Inventory &inv, int mult = 1)
	{
		for (int i = 0; i < 4; i++)
		{
			if (stones[i] < 0 && inv.stones[i] < mult * abs(stones[i]))
				return false;
		}
		return true;
	}
};

void restoreSpellAvailability(map<int, Spell> &spells)
{
	for (auto &spell : spells)
		spell.second.avail = true;
}

////////////////////////////////////// BRUTE FORCE BACKTRACER //////////////////////////////////////

/*
	this search tree algorithm brute forces all combinations but backtracks at a certain limit (= amount of steps)
	this limit is determined before this algorithm is called by a greedy best-first search algorithm\
*/

class Node
{

public:
	// root_node constructor
	Node(Inventory inv, map<int, Spell> spells, vector<int> target, int depth, int limit)
		: inv(inv), spells(spells), depth(depth)
	{
		// initialize root_node
		for (int i = 0; i < 4; i++)
			Node::stones[i] = target[i];
		Node::limit = limit;
		Node::nodes_searched = 0;
		Node::target_hits = 0;
		Node::limit_hits = 0;
		Node::error_hits = 0;

		Benchmark bm;
		bm.startBenchmark();

		if (targetHit())
			return;
		getOptions();
		if (!errorHit())
			increaseDepth();

		bm.endBenchmark();
		cerr << "Searched " << nodes_searched << " nodes in " << bm.getResult() << " ms and found ";
		cerr << target_hits << " target hits, " << limit_hits << " limit hits, " << error_hits << " error hits." << endl;
	}

	// child_node constructor
	Node(Inventory inv, map<int, Spell> spells, vector<int> log, int depth)
		: inv(inv), spells(spells), log(log), depth(depth)
	{
		Node::nodes_searched++;
		if (limitHit())
			return;
		if (targetHit())
			return;
		getOptions();
		if (!errorHit())
			increaseDepth();
	}

	////// continue writing this /////

	vector<int> getOptimalLog()
	{
		vector<vector<int>> logs = getLogs();


	}

	vector<vector<int>> getLogs()
	{
		vector<vector<int>> logs;

		if (discarded)
			return logs;
		else if (solved)
		{
			logs.emplace_back(log);
			return logs;
		}
		else
		{
			for (auto &node : nodes)
			{
				vector<vector<int>> child_logs = node.getLogs();
				if (!child_logs.empty())
				{
					for (auto &log : child_logs)
						logs.emplace_back(log);
				}
			}
			return logs;
		}
	}

private:
	/// object state
	Inventory inv;
	map<int, Spell> spells;
	vector<int> log;
	int depth;
	vector<Node> nodes;
	bool discarded = false;
	bool solved = false;
	int option_count;
	vector<int> options;
	map<int, int> repeats;
	bool rest_option = false;

	/// class state
	static int stones[4];
	static int limit;
	static int nodes_searched;
	static int target_hits;
	static int error_hits;
	static int limit_hits;

	void getOptions()
	{
		for (auto &spell : spells)
		{
			if (spell.second.haveRequiredStones(inv) && !spell.second.willOverflowInventory(inv) && spell.second.avail)
				options.push_back(spell.first);
			if (spell.second.repeat && spell.second.avail)
			{
				for (int mult = 2; mult < 10; mult++)
				{
					if (spell.second.haveRequiredStones(inv, mult) && !spell.second.willOverflowInventory(inv, mult))
						repeats.insert({spell.first, mult});
					else
						break;
				}
			}
			if (!rest_option && !spell.second.avail)
				rest_option = true;
		}
		option_count = options.size() + repeats.size();
		if (rest_option)
			option_count++;
	}

	void increaseDepth()
	{
		for (auto spell_id : options)
		{
			vector<int> mutated_log = log;
			mutated_log.emplace_back(spell_id);
			nodes.emplace_back(mutatedInventory(spell_id), mutatedSpells(spell_id), mutated_log, depth + 1);
		}
		for (auto spell : repeats)
		{
			vector<int> mutated_log = log;
			mutated_log.emplace_back(spell.first * spell.second);
			nodes.emplace_back(mutatedInventory(spell.first, spell.second), mutatedSpells(spell.first), mutated_log, depth + 1);
		}
		if (rest_option)
		{
			vector<int> mutated_log = log;
			mutated_log.push_back(NODE_REST);
			map<int, Spell> mutated_spells = spells;
			restoreSpellAvailability(mutated_spells);
			nodes.emplace_back(inv, mutated_spells, mutated_log, depth + 1);
		}
	}

	bool errorHit()
	{
		if (option_count == 0)
		{
			discarded = true;
			error_hits++;
			return true;
		}
		return false;
	}

	bool limitHit()
	{
		if (depth > limit)
		{
			discarded = true;
			limit_hits++;
			return true;
		}
		return false;
	}

	bool targetHit()
	{
		for (int i = 0; i < 4; i++)
		{
			if (inv.stones[i] < stones[i])
				return false;
		}
		if (depth < limit)
			limit = depth;
		solved = true;
		target_hits++;
		return true;
	}

	Inventory mutatedInventory(int spell_id, int mult = 1)
	{
		Inventory tmp;

		tmp = inv;
		tmp.slots_filled = 0;

		for (int i = 0; i < 4; i++)
		{
			(spells[spell_id].stones[i] <= 0) ? tmp.stones[i] -= mult * abs(spells[spell_id].stones[i]) : tmp.stones[i] += mult * spells[spell_id].stones[i];
			tmp.slots_filled += tmp.stones[i];
		}

		// cerr << "inventory mutation from: [";
		// for (int i = 0; i < 4; i++)
		// {
		// 	cerr << inv.stones[i];
		// 	(i == 3) ? cerr << "] " : cerr << ", ";
		// }
		// cerr << "to [";
		// for (int i = 0; i < 4; i++)
		// {
		// 	cerr << tmp.stones[i];
		// 	(i == 3) ? cerr << "] " : cerr << ", ";
		// }
		// cerr << "by spell [";
		// for (int i = 0; i < 4; i++)
		// {
		// 	cerr << spells[spell_id].stones[i];
		// 	(i == 3) ? cerr << "] " : cerr << ", ";
		// }
		// cerr << endl;

		return tmp;
	}

	map<int, Spell> mutatedSpells(int spell_id)
	{
		map<int, Spell> tmp;
		tmp = spells;

		tmp[spell_id].avail = false;
		return tmp;
	}

};

// static initializations
int Node::stones[4] = {0, 0, 0, 0};
int Node::limit;
int Node::nodes_searched;
int Node::target_hits;
int Node::error_hits;
int Node::limit_hits;

////////////////////////////////////// GLOBALS //////////////////////////////////////

map<int, Recipe> g_recipes;
map<int, Tome> g_tomes;
map<int, Spell> g_spells;
Inventory g_inv;
int enemy_score;

////////////////////////////////////// RECIPE MECHANICS //////////////////////////////////////

void printLogs(int recipe_id, vector<vector<int>> results)
{
	Recipe recipe = g_recipes[recipe_id];

	cerr << results.size() << " logs for recipe " << recipe.id << " [";
	for (int i = 0; i < 4; i++)
	{
		cerr << recipe.stones[i];
		(i == 3) ? cerr << "] " : cerr << ", ";
	}
	cerr << "missing [";
	vector<int> missing = recipe.getMissingStones(g_inv);
	for (int i = 0; i < 4; i++)
	{
		cerr << missing[i];
		(i == 3) ? cerr << "]" : cerr << ", ";
	}
	cerr << ":" << endl;
	for (auto log : results)
	{
		cerr << log.size() << " steps: ";
		for (auto step : log)
		{
			if (step != 777)
				cerr << step;
			else
				cerr << "REST";
			cerr << ", ";
		}
		cerr << endl;
	}
}

void searchRecipes()
{
	for (auto recipe : g_recipes)
	{
		Node root_node(g_inv, g_spells, recipe.second.getMissingStones(g_inv), 0, NODE_DEPTH_LIMIT);

		vector<vector<int>> results = root_node.getLogs();
		printLogs(recipe.second.id, results);
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
	g_recipes.clear();
	g_spells.clear();
	g_tomes.clear();

	processActions();
	processInventory();
}

//////////////////////////////////////////////// DEBUG /////////////////////////////////////////

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
		else if (tome.second.mutator)
			cerr << " - mutator";
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
	printTomes();
	printSpells();
}

////////////////////////////////////// HIGH LEVEL CONTROL FLOW ///////////////////////////////////

int getFreeTome()
{
	for (auto tome : g_tomes)
	{
		if (tome.second.tax_cost == 0)
			return tome.first;
	}
	return -1;
}

void printAction()
{
	cout << "WAIT" << endl;
}

int main()
{
	while (true)
	{
		processInput();
		//printData();
		searchRecipes();
		printAction();
	}
}