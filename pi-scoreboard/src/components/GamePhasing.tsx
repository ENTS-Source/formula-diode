import * as React from 'react';
import { useState } from 'react';
import { GamePhase } from "../models/GameState";
import { useGameState } from "../hooks/useGameState";
import { Player } from "./Player";

import './GamePhasing.css';

export function GamePhasing() {
    const [phase, setPhase] = useState<GamePhase>("intro");
    const [players, setPlayers] = useState<number>(2);

    useGameState((state) => {
        if (phase !== state.phase) setPhase(state.phase);
        if (players !== state.players.length) setPlayers(state.players.length);
    });

    switch(phase) {
        case "intro":
            return <div className='fd_gameNotice'>
                <h1>Get ready!</h1>
            </div>;
        case "error":
            return <div className='fd_gameNotice'>
                <h1 style={{color: 'tomato'}}>ERROR: PLEASE RESTART GAME</h1>
            </div>;
        case "running":
        case "end":
        default:
            break;
    }

    const playerNodes: React.ReactNode[] = [];
    for (let i = 0; i < players; i++) {
        playerNodes.push(<Player id={i} key={i} />);
    }
    return <div className='fd_gameState'>
        <h2 className='fd_titleText'>Player Times</h2>
        <div className='fd_scores'>
            { playerNodes }
        </div>
    </div>;
}
