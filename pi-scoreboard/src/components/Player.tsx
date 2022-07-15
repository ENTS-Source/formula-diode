import * as React from 'react';
import { useState } from 'react';
import { useGameState } from "../hooks/useGameState";
import { Stopwatch } from "./Stopwatch";

export function Player({ id }: { id: number }) {
    const [startTime, setStartTime] = useState<number>(Date.now());
    const [winner, setWinner] = useState<boolean>(false);
    const [lapTimes, setLapTimes] = useState<number[]>([]);
    const [finishTime, setFinishTime] = useState<number>(-1);

    useGameState((state) => {
        if (startTime !== state.startTimeMs) setStartTime(state.startTimeMs);

        const player = state.players[id];
        if (winner !== player.winner) setWinner(player.winner);
        if (lapTimes.length !== player.lapsTimesMs.length) setLapTimes([...player.lapsTimesMs]);
        if (player.finished) {
            if (finishTime !== player.totalTimeMs) setFinishTime(player.totalTimeMs)
        } else {
            if (finishTime >= 0) setFinishTime(-1);
        }
    });

    const time: React.ReactNode = finishTime >= 0
        ? (finishTime / 1000).toFixed(2)
        : <Stopwatch startTime={startTime} />;
    const lapTime: React.ReactNode = finishTime < 0
        ? <Stopwatch startTime={startTime + lapTimes.reduce((p, c) => p + c, 0)} />
        : null;
    return <div className={`fd_score ${winner ? 'fd_winner' : ''}`}>
        <span className='fd_player'>Player { id + 1 }</span>
        <span className='fd_time'>{ time }</span>
        { winner ? <span className='fd_star'>🥇</span> : null }
        <div className='fd_lapTimes'>
            {
                lapTimes.map((t, i) => <div>
                    <span className='fd_lap'>Lap { i + 1 }</span>
                    <span className='fs_time'>{ (t / 1000).toFixed(2) }</span>
                </div>)
            }
            {
                finishTime < 0
                    ? <div>
                        <span className='fd_lap'>Lap { lapTimes.length + 1 } </span>
                        <span className='fs_time'>{ lapTime }</span>
                    </div>
                    : null
            }
        </div>
    </div>;
}
