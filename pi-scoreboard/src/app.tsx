import * as React from 'react';
import * as ReactDOM from 'react-dom';

import './app.css';

import { TopScores } from './TopScores';
import { LastGame } from './LastGame';

function render() {
    ReactDOM.render(<>
        <h1 className='fd_title fd_titleText'>Formula Diode</h1>
        <TopScores />
        <LastGame />
    </>, document.body);
}

render();