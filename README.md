# fall_challenge_2020
My bot's source code for CodinGame's Fall Challenge 2020.

<a href="https://ibb.co/Wpckr89"><img src="https://i.ibb.co/YTtc5n9/Screenshot-from-2020-11-14-15-27-09.png" alt="Screenshot-from-2020-11-14-15-27-09" border="0"></a>

# version 1

My first version's strategy is based on a weight distribution system for the different available stones. <br>
The weight is based on the amount of spells it takes to get a single unit of each stone. <br>
The efficiency of each recipe is calculated based on the weight distribution of the ingredients, the spread of different ingredients and the reward for brewing the recipe. <br>
New spells are only acquired from the tomebook if they are either free resources (freeloaders) or meet specific requirements to be efficient mutators. <br>
To be qualified as mutator spells need to have both limited spread in ingredient costs and ingredient rewards and their exchange of ingredients would need to be above a certain minimum efficiency. <br>
This efficiency is based upon the weighted value of the ingredients exchanged. <br>

My bot first looks for eligible spells in the tomebook, if it can't find any it tries to brew the most efficient potion. <br>
If the required resources are not available, it looks for the most efficient spell to acquire those resources. <br>
If the required resources are not available for that spell, it recursively looks for the most efficient spell to acquire those resources.

This code has gotten me to around #100 position in the competition. <br>

Key flaws of version 1: <br>

- the amount of steps (including resting in between spells) were not taken into account while calculating a recipe or spell's efficiency
- greedy algorithm, no complete computation of all steps, only locally best option
- circular dependencies if I wanted to implement stone value dependencies in mutator spells

<br>

# version 2

During the 3rd day of the competition I reached the limit of the strategy I was using and started working on a new strategy to optimize my decision making. <br> <br>
It became obvious to me that I needed a graph / tree searching algorithm for optimal results. 
