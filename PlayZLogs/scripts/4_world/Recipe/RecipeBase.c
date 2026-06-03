modded class RecipeBase
{
	override void PerformRecipe(ItemBase item1, ItemBase item2, PlayerBase player)
	{
		// We need to capture the state before the recipe ruins ingredients
		// RecipeBase::PerformRecipe handles ingredient sorting and spawning results
		
		if (GetGame().IsServer())
		{
			// The original PerformRecipe sorts ingredients internally.
			// To be safe and efficient, we'll log at the start of the process.
			// However, we don't have the results yet.
			// Let's call super first, then log if successful.
			
			// Ingredients sorted array is filled during CheckRecipe (called inside PerformRecipe)
			// Results are available after super.PerformRecipe
		}
		
		super.PerformRecipe(item1, item2, player);
		
		if (GetGame().IsServer())
		{
			// Convert static array m_IngredientsSorted to dynamic array
			array<ItemBase> ingredients = new array<ItemBase>;
			for (int i = 0; i < 2; i++) // MAX_NUMBER_OF_INGREDIENTS is usually 2
			{
				if (m_IngredientsSorted[i]) ingredients.Insert(m_IngredientsSorted[i]);
			}
			
			PlayZLogData_Recipe data = new PlayZLogData_Recipe();
			data.InitRecipe(this, player, ingredients, null);
			PlayZLogger.LogRecipe("RecipePerformed", data);
		}
	}
}
