class PlayZStashDigResolve
{
	protected static const float HIDDEN_STASH_SEARCH_RADIUS = 1.0;
	protected static const float DIG_IN_STASH_SEARCH_RADIUS = 2.0;

	static bool IsStashEntity(Object obj)
	{
		if (!obj)
			return false;

		return obj.IsInherited(UndergroundStash);
	}

	static UndergroundStash CastStash(Object obj)
	{
		if (!IsStashEntity(obj))
			return null;

		return UndergroundStash.Cast(obj);
	}

	static UndergroundStash ResolveStashFromTarget(ActionTarget target)
	{
		if (!target)
			return null;

		UndergroundStash directStash = CastStash(target.GetObject());
		if (directStash)
			return directStash;

		vector cursorPosition = target.GetCursorHitPos();
		return FindNearestStashAt(cursorPosition, HIDDEN_STASH_SEARCH_RADIUS);
	}

	static UndergroundStash ResolveStashAfterDigIn(EntityAI buriedItem, vector nearPos)
	{
		if (buriedItem)
		{
			UndergroundStash parentStash = CastStash(buriedItem.GetHierarchyParent());
			if (parentStash)
				return parentStash;

			InventoryLocation itemLoc = new InventoryLocation();
			if (buriedItem.GetInventory().GetCurrentInventoryLocation(itemLoc))
			{
				EntityAI locParent = itemLoc.GetParent();
				parentStash = CastStash(locParent);
				if (parentStash)
					return parentStash;
			}
		}

		return FindNearestStashAt(nearPos, DIG_IN_STASH_SEARCH_RADIUS);
	}

	protected static UndergroundStash FindNearestStashAt(vector position, float radius)
	{
		array<Object> nearbyObjects = new array<Object>();
		array<CargoBase> proxyCargos = new array<CargoBase>();
		g_Game.GetObjectsAtPosition3D(position, radius, nearbyObjects, proxyCargos);

		int nearbyCount = nearbyObjects.Count();
		for (int i = 0; i < nearbyCount; i++)
		{
			UndergroundStash nearbyStash = CastStash(nearbyObjects[i]);
			if (nearbyStash)
				return nearbyStash;
		}

		return null;
	}
}
