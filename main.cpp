#include <iostream>
#include <string>
#include <map>

using namespace std;

struct Inventory
{
	int blue;
	int green;
	int orange;
	int yellow;
	int score;

    Inventory() {}
	Inventory(int inv0, int inv1, int inv2, int inv3, int score)
		: blue(inv0), green(inv1), orange(inv2), yellow(inv3), score(score){};
};

struct Recipe
{
	int blue;
	int green;
	int orange;
	int yellow;
	int rupees;
	int weighted;
	float profit;

	Recipe(int delta0, int delta1, int delta2, int delta3, int price) : rupees(price)
	{
		blue = abs(delta0);
		green = abs(delta1);
		orange = abs(delta2);
		yellow = abs(delta3);
		weighted = 1 * blue + 2 * green + 3 * orange + 4 * yellow;
		profit = (float)price / weighted;
	}
};

struct Spell
{
	int blue;
	int green;
	int orange;
	int yellow;
	int avail;

	Spell(int delta0, int delta1, int delta2, int delta3, int avail)
		: blue(delta0), green(delta1), orange(delta2), yellow(delta3), avail(avail){};
};

Inventory inv;
map<int, Recipe> recipes;
map<int, Spell> spells;
bool spells_set = false;
string output;

void printData()
{
	cerr << "-------------SPELLS-------------" << endl;
	for (auto x : spells)
	{
		cerr << x.first << ".blue: " << x.second.blue << endl;
		cerr << x.first << ".green: " << x.second.green << endl;
		cerr << x.first << ".orange: " << x.second.orange << endl;
		cerr << x.first << ".yellow: " << x.second.yellow << endl;
	}
	cerr << "-------------RECIPES-------------" << endl;
	for (auto x : recipes)
	{
		cerr << x.first << ".blue: " << x.second.blue << endl;
		cerr << x.first << ".green: " << x.second.green << endl;
		cerr << x.first << ".orange: " << x.second.orange << endl;
		cerr << x.first << ".yellow: " << x.second.yellow << endl;
		cerr << x.first << ".rupees: " << x.second.rupees << endl;
        cerr << x.first << ".weighted: " << x.second.weighted << endl;
        cerr << x.first << ".profit: " << x.second.profit << endl;

	}
	cerr << "-------------INVENTORY-------------" << endl;
	cerr << "blue: " << inv.blue << endl;
	cerr << "green: " << inv.green << endl;
	cerr << "orange: " << inv.orange << endl;
	cerr << "yellow: " << inv.yellow << endl;
	cerr << "score: " << inv.score << endl;
}

void processActions()
{
	int actionCount; // the number of spells and recipes in play
	cin >> actionCount;

	for (int i = 0; i < actionCount; i++)
	{
		int actionId;	   // the unique ID of this spell or recipe
		string actionType; // in the first league: BREW; later: CAST, OPPONENT_CAST, LEARN, BREW
		int delta0;		   // tier-0 ingredient change
		int delta1;		   // tier-1 ingredient change
		int delta2;		   // tier-2 ingredient change
		int delta3;		   // tier-3 ingredient change
		int price;		   // the price in rupees if this is a potion
		bool castable;	   // in the first league: always 0; later: 1 if this is a castable player spell

		int tomeIndex;	 // in the first two leagues: always 0; later: the index in the tome if this is a tome spell, equal to the read-ahead tax
		int taxCount;	 // in the first two leagues: always 0; later: the amount of taxed tier-0 ingredients you gain from learning this spell
		bool repeatable; // for the first two leagues: always 0; later: 1 if this is a repeatable player spell

		cin >> actionId >> actionType >> delta0 >> delta1 >> delta2 >> delta3 >> price >> tomeIndex >> taxCount >> castable >> repeatable;

		if (actionType == "BREW" || "CAST")
		{
			if (actionType == "BREW")
				recipes.emplace(actionId, Recipe(delta0, delta1, delta2, delta3, price));
			else if (actionType == "CAST" && !spells_set)
				spells.emplace(actionId, Spell(delta0, delta1, delta2, delta3, castable));
		}
	}
}

void processInventory()
{
	// i == 0 is your own inventory
	for (int i = 0; i < 2; i++)
	{
		int inv0; // tier-0 ingredients in inventory
		int inv1;
		int inv2;
		int inv3;
		int score; // amount of rupees
		cin >> inv0 >> inv1 >> inv2 >> inv3 >> score;
		if (i == 0)
			inv = Inventory(inv0, inv1, inv2, inv3, score);
	}
}

void processInput()
{
	recipes.clear();
	processActions();
	if (!spells_set)
		spells_set = true;
	processInventory();
}

void computeOutput()
{
	// BREW <id> | CAST <id> | LEARN <id> | REST | WAIT

	output = "REST";
	cout << output << endl;
}

int main()
{
	while (true)
	{
		processInput();
		printData();
		computeOutput();
	}
}