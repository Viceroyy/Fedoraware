#include "SpectatorList.h"

#include "../../Vars.h"
#include "../Menu.h"

bool CSpectatorList::GetSpectators(CBaseEntity* pLocal)
{
	Spectators.clear();

	for (const auto& pPlayer : g_EntityCache.GetGroup(EGroupType::PLAYERS_ALL))
	{
		CBaseEntity* pObservedPlayer = I::ClientEntityList->GetClientEntityFromHandle(
			pPlayer->GetObserverTarget());

		if (pPlayer && !pPlayer->IsAlive() && pObservedPlayer == pLocal)
		{
			std::wstring szMode;
			switch (pPlayer->GetObserverMode())
			{
				case OBS_MODE_FIRSTPERSON:
				{
					szMode = L"1st";
					break;
				}
				case OBS_MODE_THIRDPERSON:
				{
					szMode = L"3rd";
					break;
				}
				case OBS_MODE_DEATHCAM:
				{
					szMode = L"Deathcam";
					break;
				}
				case OBS_MODE_FREEZECAM:
				{
					szMode = L"Freezecam";
					break;
				}
				default: continue;
			}

			PlayerInfo_t playerInfo{ };
			if (I::EngineClient->GetPlayerInfo(pPlayer->GetIndex(), &playerInfo))
			{
				Spectators.push_back({
					Utils::ConvertUtf8ToWide(playerInfo.name), szMode, g_EntityCache.IsFriend(pPlayer->GetIndex()),
					pPlayer->GetTeamNum(), pPlayer->GetIndex()
									 });
			}
		}
	}

	return !Spectators.empty();
}

bool CSpectatorList::ShouldRun()
{
	return Vars::Visuals::SpectatorList.Value/* && !I::EngineVGui->IsGameUIVisible()*/;
}

void CSpectatorList::Draw()
{
	if (!ShouldRun()) { return; }

	if (Vars::Visuals::SpectatorList.Value == 1) { DrawDefault(); }
	else { DrawClassic(); }
}

void CSpectatorList::DragSpecList(int& x, int& y, int w, int h, int offsety)
{
	if (!F::Menu.IsOpen) { return; }

	int mousex, mousey;
	I::VGuiSurface->SurfaceGetCursorPos(mousex, mousey);

	static POINT delta{};
	static bool drag = false;
	static bool move = false;
	const bool held = GetKey(VK_LBUTTON);

	if ((mousex > x && mousex < x + w && mousey > y - offsety && mousey < y - offsety + h) && held)
	{
		drag = true;

		if (!move)
		{
			delta.x = mousex - x;
			delta.y = mousey - y;
			move = true;
		}
	}

	if (drag)
	{
		x = mousex - delta.x;
		y = mousey - delta.y;
	}

	if (!held)
	{
		drag = false;
		move = false;
	}
}

void CSpectatorList::DrawDefault()
{
	DragSpecList(
		SpecListX,
		SpecListY,
		SpecListW,
		SpecListTitleBarH,
		0);

	const auto& menuFont = g_Draw.GetFont(FONT_MENU);

	// 38 to 43
	g_Draw.Rect(SpecListX, SpecListY, SpecListW, SpecListTitleBarH, { 43, 43, 45, 250 });

	g_Draw.String(menuFont,
				  SpecListX + (SpecListW / 2),
				  SpecListY + (SpecListTitleBarH / 2),
				  Vars::Menu::Colors::MenuAccent.Value,
				  ALIGN_CENTER,
				  "%hs", "Spectators");

	if (const auto& pLocal = g_EntityCache.GetLocal())
	{
		if (!pLocal->IsAlive() || !GetSpectators(pLocal)) { return; }

		constexpr int nSpacing = 5;
		constexpr int nModeW = 15;
		const int nFontTall = menuFont.nTall;
		const int nModeX = SpecListX + nSpacing;
		const int nNameX = nModeX + nModeW + (nSpacing * 2);
		int y = SpecListY + SpecListTitleBarH;
		const int h = nFontTall * Spectators.size();

		// 25 to 31
		g_Draw.Rect(SpecListX, y, SpecListW, h, { 36, 36, 36, 255 });
		g_Draw.Line(nModeX + nSpacing + nModeW, y, nModeX + nSpacing + nModeW, y + h - 1, { 255, 255, 255, 255 });

		for (size_t n = 0; n < Spectators.size(); n++)
		{
			if (Spectators[n].Name.length() > 20)
			{
				Spectators[n].Name.replace(20, Spectators[n].Name.length(), L"...");
			}

			y = SpecListY + SpecListTitleBarH + (nFontTall * n);

			g_Draw.String(menuFont, nModeX, y, { 255, 255, 255, 255 }, ALIGN_DEFAULT, Spectators[n].Mode.data());
			g_Draw.String(menuFont, nNameX, y, { 255, 255, 255, 255 }, ALIGN_DEFAULT, Spectators[n].Name.data());
		}
	}
}

void CSpectatorList::DrawClassic()
{
	if (const auto& pLocal = g_EntityCache.GetLocal())
	{
		if (!pLocal->IsAlive() || !GetSpectators(pLocal)) { return; }

		int nDrawY = (g_ScreenSize.h / 2) - 300;
		const int centerr = g_ScreenSize.c;
		const auto& nameFont = g_Draw.GetFont(FONT_ESP_NAME);
		const int addyy = nameFont.nTall;

		g_Draw.String(
			nameFont,
			centerr, nDrawY - addyy,
			{ 255, 255, 255, 255 },
			ALIGN_CENTERHORIZONTAL,
			L"Spectating You:");

		for (const auto& Spectator : Spectators)
		{
			int nDrawX = g_ScreenSize.c;

			int w, h;
			I::VGuiSurface->GetTextSize(nameFont.dwFont, (Spectator.Mode + Spectator.Name).c_str(), w, h);

			const int nAddY = nameFont.nTall;
			if (Vars::Visuals::SpectatorList.Value == 3)
			{
				PlayerInfo_t pi{};
				if (!I::EngineClient->GetPlayerInfo(Spectator.Index, &pi)) { continue; }

				g_Draw.Avatar(nDrawX - (w / 2) - 25, nDrawY, 24, 24, pi.friendsID);
				// center - half the width of the string
				nDrawY += 6;
			}

			g_Draw.String(
				nameFont,
				nDrawX, nDrawY,
				Spectator.IsFriend
				? Vars::Colours::Friend.Value
				: GetTeamColour(Spectator.Team, Vars::ESP::Main::EnableTeamEnemyColors.Value),
				ALIGN_CENTERHORIZONTAL,
				L"[%ls] %ls", Spectator.Mode.data(), Spectator.Name.data());

			nDrawY += nAddY;
		}
	}
}
