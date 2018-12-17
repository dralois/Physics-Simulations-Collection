﻿#include "pch.h"

#include "SphereSystem.h"

#pragma region Functions

#pragma region Internal

// Sortiert alle Bälle in ensprechende Zellen ein
vector<int> SphereSystem::X_SortBalls()
{
	vector<int> notEmpty;
	// Alle Bälle sortieren
	for(auto ball = m_Balls.begin(); ball != m_Balls.end(); ball++)
	{
		int x = floorf(ball->Position.x / ball->Radius);
		int y = floorf(ball->Position.z / ball->Radius);
		int index = ((y * m_iGridWidth) + x) * MAXCOUNT;
		// In passender Zelle speichern falls möglich
		if(m_GridOccupation[index] < MAXCOUNT)
		{
			m_GridAccelerator[index + m_GridOccupation[index]++] = &*ball;
			// Belegten Index ggf. speichern
			if (find(notEmpty.begin(), notEmpty.end(), index) == notEmpty.end())
				notEmpty.push_back(index);
		}
	}
	// Gebe belegte Zellen zurück
	return notEmpty;
}

// überprüfe Zellennachbarn auf Kollisionen
vector<int> SphereSystem::X_CheckNeighbors(int pi_iCell)
{
	vector<int> neighbors;
	// Index berechnen
	int cellX = pi_iCell % m_iGridWidth;
	int cellY = pi_iCell / m_iGridWidth;
	// überprüfe Nachbarn
	for(int x = -1; x < 2; x++)
	{
		for(int y = -1; y < 2; y++)
		{
			// Eigene Zelle ignorieren
			if (x == y == 0)
				continue;
			// Randfälle behandeln
			if (cellX + x < 0 || cellX + x >= m_iGridWidth)
				continue;
			// m_GridOccupation.size() / m_iGridWidth ist eigentlich gleich m_iGridWidth da Quadrat
			if (cellY + y < 0 || cellY + y >= m_GridOccupation.size() / m_iGridWidth)
				continue;
			// Speichere alle Nachbarn in Array
			int index = ((cellY + y) * m_iGridWidth) + (cellX + x);
			for(int j = 0; j < m_GridOccupation[index]; j++)
			{
				neighbors.push_back(index + j);
			}
		}
	}
	// Gebe Nachbarnliste zurück
	return neighbors;
}

// Bälle dürfen Box nicht verlassen
void SphereSystem::X_ApplyBoundingBox(Ball & ball)
{
	// Positive Richtung clampen und Ball zurückspringen
	if (ball.Position.x > m_v3BoxSize.x) 
	{
		ball.Position.x = m_v3BoxSize.x;
		ball.Velocity = Vec3(-1.0 * ball.Velocity.x, ball.Velocity.y, ball.Velocity.z);
	}
	if (ball.Position.y > m_v3BoxSize.y)
	{
		ball.Position.y = m_v3BoxSize.y;
		ball.Velocity = Vec3(ball.Velocity.x, -1.0f * ball.Velocity.y, ball.Velocity.z);
	}
	if (ball.Position.z > m_v3BoxSize.z)
	{
		ball.Position.z = m_v3BoxSize.z;
		ball.Velocity = Vec3(ball.Velocity.x, ball.Velocity.y, -1.0f * ball.Velocity.z);
	}
	// Negative Richtung clampen
	if (ball.Position.x < 0.0f)
	{
		ball.Position.x = 0.0f;
		ball.Velocity = Vec3(-1.0 * ball.Velocity.x, ball.Velocity.y, ball.Velocity.z);
	}
	if (ball.Position.y < 0.0f)
	{
		ball.Position.y = 0.0f;
		ball.Velocity = Vec3(ball.Velocity.x, -1.0f * ball.Velocity.y, ball.Velocity.z);
	}
	if (ball.Position.z < 0.0f)
	{
		ball.Position.z = 0.0f;
		ball.Velocity = Vec3(ball.Velocity.x, ball.Velocity.y, -1.0f * ball.Velocity.z);
	}
}

// überprüft auf Kollision und updatet Kräfte
void SphereSystem::X_ApplyCollision(Ball & ball1, Ball & ball2, const function<float(float)>& kernel, float fScaler)
{
	// Zum Achten: man braucht nur die Kraft für Ball 1 aktualisieren, nicht für Ball 2, sonst ist jede Kraft zweimal berechnet
	// Für Kollisionen am Anfang vom Zeitschritt
	float dist = sqrtf(ball1.Position.squaredDistanceTo(ball2.Position));
	if(dist <= ball1.Radius + ball2.Radius)
	{
		Vec3 collNorm = getNormalized(ball1.Position - ball2.Position);
		ball1.Force += fScaler * kernel(dist / ball1.Radius * 2.0f) * collNorm;
	}

	// Für Kollisionen in der Mitte vom Zeitschritt
	float distTilde = sqrtf(ball1.PositionTilde.squaredDistanceTo(ball2.PositionTilde));
	if (distTilde <= ball1.Radius + ball2.Radius)
	{
		Vec3 collNormTilde = getNormalized(ball1.Position - ball2.Position);
		ball1.ForceTilde += fScaler * kernel(dist / ball1.Radius * 2.0f) * collNormTilde;
	}
}

#pragma endregion

// Rendert Bälle in übergebener Farbe
void SphereSystem::drawFrame(DrawingUtilitiesClass* DUC, const Vec3& v3Color)
{
	// Farbe einrichten
	DUC->setUpLighting(Vec3(), 0.6f*Vec3(1.0f), 100.0f, v3Color);
	// Bälle rendern
	for(auto ball = m_Balls.begin(); ball != m_Balls.end(); ball++)
	{
		DUC->drawSphere(ball->Position, Vec3(ball->Radius));
	}
}

// Externe Kräte anwenden (z.B. Maus, Gravitation, Damping)
// TODO Demo 1,2,3

// Problem: WTF braucht man timeElapsed für diese Methode wenn Mauskraft schon angegeben ist???
void SphereSystem::externalForcesCalculations(float timeElapsed, Vec3 v3MouseForce)
{
	for (auto ball = m_Balls.begin(); ball != m_Balls.end(); ball++)
	{
		ball->Force += Vec3(0, -1.0f * m_fGravity * ball->Mass, 0);
		ball->Force += v3MouseForce;
		ball->Force -= m_fDamping * ball->Velocity;

		ball->ForceTilde += Vec3(0, -1.0f * m_fGravity * ball->Mass, 0);
		ball->ForceTilde += v3MouseForce;
		// Damping für Midpoint muss man später berechnen, da man braucht noch Midpoint Velocity
	}
}

// Simuliert einen Zeitschritt
// TODO Demo 1,2,3
void SphereSystem::simulateHalfTimestep(float timeStep)
{
	//Berechnen aller Positionen nach h/2 Schritt
	for (auto ball = m_Balls.begin(); ball != m_Balls.end(); ball++)
	{
		ball->PositionTilde = ball->Position + ball->Velocity * timeStep / 2.0f;
	}
}
void SphereSystem::simulateTimestep(float timeStep)
{
	// Aktualisiere Geschwindigkeit/Position
	for (auto ball = m_Balls.begin(); ball != m_Balls.end(); ball++)
	{
		Vec3 midPointVelocity = ball->Velocity + (ball->Force / ball->Mass) * timeStep / 2.0f;
		ball->Position += timeStep * midPointVelocity;
		ball->ForceTilde -= m_fDamping * midPointVelocity;
		ball->Velocity += timeStep * (ball->ForceTilde / ball->Mass) ;
		// Kraft zurücksetzen
		ball->Force = Vec3(0.0f);
		ball->ForceTilde = Vec3(0.0f);
		// Als letztes Position clampen
		X_ApplyBoundingBox(*ball);
	}
}

// Löst Kollisionen auf
void SphereSystem::collisionResolve(const function<float(float)>& kernel, float fScaler)
{
	switch (m_iAccelerator)
	{
	// Naiver Ansatz: Alles mit allem
	case 0:
	{
		for(auto ball = m_Balls.begin(); ball != m_Balls.end(); ball++)
		{
			for (auto coll = m_Balls.begin(); coll != m_Balls.end(); coll++)
			{
				// Verhindere Kollision mit selbst
				if (ball != coll)
				{
					// Löse Kollision auf falls es eine gibt
					X_ApplyCollision(*ball, *coll, kernel, fScaler);
				}
			}
		}
		break;
	}
	// Grid Acceleration
	case 1:
	{
		// Sortiere Bälle in Zellen, speichere belegte Zellen
		vector<int> toCheck = X_SortBalls();
		// Für alle belegten Zellen
		for(int i = 0; i < toCheck.size(); i++)
		{
			// Bestimme Nachbarindizes
			vector<int> neighbors = X_CheckNeighbors(toCheck[i]);
			// Für alle Bälle in der Zelle
			for(int k = 0; k < m_GridOccupation[toCheck[i]]; k++)
			{
				// Für alle Nachbarn
				for(int j = 0; j < neighbors.size(); j++)
				{
					// Löse Kollision auf
					X_ApplyCollision(	*m_GridAccelerator[toCheck[i] + k],
														*m_GridAccelerator[neighbors[j]],
														kernel, fScaler);
				}
			}
		}
		break;
	}
	case 2:
	{
		cout << "KD currently not supported!" << endl;
		break;
	}
	default:
		cout << "No valid accelerator selected!" << endl;
		break;
	}
}

#pragma endregion

#pragma region Initialisation

// Initialsiert neues Ballsystem
SphereSystem::SphereSystem(	int pi_iAccelerator, int pi_iNumSpheres,
														float pi_fRadius, float pi_fMass) :
	m_iAccelerator(pi_iAccelerator)
{
	// Zellenanzahl bestimmen
	float cellAmount = ((float)(pi_iNumSpheres)) / ((float)(MAXCOUNT));
	int gridDim = ceil(sqrtf(cellAmount));
	// Als Box speichern
	m_v3BoxSize = Vec3(gridDim * pi_fRadius);
	// Grid erstellen
	m_iGridWidth = gridDim;
	m_GridOccupation.resize(gridDim * gridDim);
	m_GridAccelerator.resize(gridDim * gridDim * MAXCOUNT);
	// Bälle erstellen
	for(int i = 0; i < pi_iNumSpheres; i++)
	{
		Ball newBall;
		// Erstelle Ball
		newBall.Force = newBall.ForceTilde = newBall.PositionTilde = Vec3(0.0f);
		// Spawne Bälle in einem Matrixraster
		newBall.Position = Vec3(floorf(((float) i) / gridDim),
			((float)i) - (floorf(((float)i) / gridDim) * gridDim), gridDim);
		newBall.Radius = pi_fRadius;
		newBall.Mass = pi_fMass;
		// Im Array speichern
		m_Balls.push_back(newBall);
	}
}

// Aufräumen
SphereSystem::~SphereSystem()
{
	m_GridAccelerator.clear();
	m_GridOccupation.clear();
	m_Balls.clear();
}

#pragma endregion
