import * as React from 'react';
import { useState } from 'react';
import { useGameState } from "../hooks/useGameState";
import { GamePhase } from "../models/GameState";

import './TopScores.css';

const DEFAULT_SCORES = [-1000, -1000, -1000, -1000, -1000]; // also defines length

export function TopScores() {
    const [scores, setScores] = useState(DEFAULT_SCORES);
    const [phase, setPhase] = useState<GamePhase>("intro");

    useGameState((state) => {
        if (phase !== state.phase) {
            setPhase(state.phase);

            if (state.phase === "end" && !state.automated) {
                let newScores = [...scores];
                for (const player of state.players) {
                    newScores.push(player.totalTimeMs);
                }
                newScores.sort((a, b) => {
                    if (a == b && a < 0) return 0;
                    if (a < 0 && b >= 0) return 1;
                    if (a >= 0 && b < 0) return -1;
                    return a - b;
                });
                newScores = newScores.slice(0, DEFAULT_SCORES.length);
                setScores(newScores);
            }
        }
    });

    return <div className='fd_topScores'>
        <h2 className='fd_titleText'>Best Times</h2>
        <div className='fd_scores'>
            { scores.map((s, i) => <div className='fd_score' key={i}>
                { (s / 1000).toFixed(2) }
            </div>) }
        </div>
    </div>;
}
