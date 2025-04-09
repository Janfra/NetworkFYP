

#pragma once

/**
 * 
 */
namespace NetworkUtils 
{
	/// <summary>
	/// Used in cases where P2P logic and non-P2P logic are mixed for easier readability and mantainability 
	/// </summary>
	/// <returns>Whether we are in P2P mode, this is decided at compile time. Will never change during runtime.</returns>
	constexpr bool IsP2PMode() { return P2PMODE; }
}
