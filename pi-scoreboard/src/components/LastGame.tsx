import * as React from 'react';

import './LastGame.css';

export class LastGame extends React.PureComponent {
    public render() {
        return <div className='fd_lastGame'>
            <h2 className='fd_titleText'>Last Game</h2>
            <div className='fd_scores'>
                <div className='fd_score'>
                    <span className='fd_player'>Player 1</span>
                    <span className='fd_time'>1:00</span>
                    <span className='fd_star'>🥉</span>
                    <div className='fd_lapTimes'>
                        <div>
                            <span className='fd_lap'>Lap 1</span>
                            <span className='fs_time'>1:00</span>
                        </div>
                        <div>
                            <span className='fd_lap'>Lap 2</span>
                            <span className='fs_time'>1:00</span>
                        </div>
                        <div>
                            <span className='fd_lap'>Lap 3</span>
                            <span className='fs_time'>1:00</span>
                        </div>
                    </div>
                </div>
                <div className='fd_score fd_winner'>
                    <span className='fd_player'>Player 2</span>
                    <span className='fd_time'>1:00</span>
                    <span className='fd_star'>🥇</span>
                    <div className='fd_lapTimes'>
                        <div>
                            <span className='fd_lap'>Lap 1</span>
                            <span className='fs_time'>1:00</span>
                        </div>
                        <div>
                            <span className='fd_lap'>Lap 2</span>
                            <span className='fs_time'>1:00</span>
                        </div>
                        <div>
                            <span className='fd_lap'>Lap 3</span>
                            <span className='fs_time'>1:00</span>
                        </div>
                    </div>
                </div>
                <div className='fd_score'>
                    <span className='fd_player'>Player 3</span>
                    <span className='fd_time'>1:00</span>
                    <span className='fd_star'>🥈</span>
                    <div className='fd_lapTimes'>
                        <div>
                            <span className='fd_lap'>Lap 1</span>
                            <span className='fs_time'>1:00</span>
                        </div>
                        <div>
                            <span className='fd_lap'>Lap 2</span>
                            <span className='fs_time'>1:00</span>
                        </div>
                        <div>
                            <span className='fd_lap'>Lap 3</span>
                            <span className='fs_time'>1:00</span>
                        </div>
                    </div>
                </div>
                <div className='fd_score'>
                    <span className='fd_player'>Player 4</span>
                    <span className='fd_time'>1:00</span>
                    <div className='fd_lapTimes'>
                        <div>
                            <span className='fd_lap'>Lap 1</span>
                            <span className='fs_time'>1:00</span>
                        </div>
                        <div>
                            <span className='fd_lap'>Lap 2</span>
                            <span className='fs_time'>1:00</span>
                        </div>
                        <div>
                            <span className='fd_lap'>Lap 3</span>
                            <span className='fs_time'>1:00</span>
                        </div>
                    </div>
                </div>
            </div>
        </div>;
    }
}
