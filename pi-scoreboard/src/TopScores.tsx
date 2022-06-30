import * as React from 'react';

import './TopScores.css';

export class TopScores extends React.PureComponent {
    public render() {
        return <div className='fd_topScores'>
            <h2 className='fd_titleText'>Top Scores</h2>
            <div className='fd_scores'>
                <div className='fd_score'>
                    1:00 as Player 1
                </div>
                <div className='fd_score'>
                    1:00 as Player 1
                </div>
                <div className='fd_score'>
                    1:00 as Player 1
                </div>
                <div className='fd_score'>
                    1:00 as Player 1
                </div>
                <div className='fd_score'>
                    1:00 as Player 1
                </div>
            </div>
        </div>;
    }
}