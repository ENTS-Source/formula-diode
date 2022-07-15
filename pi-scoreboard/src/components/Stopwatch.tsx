import * as React from 'react';
import { useEffect, useState } from 'react';

export function Stopwatch({ startTime }: { startTime: number }) {
    const [time, setTime] = useState<number>();

    useEffect(() => {
        const timerId = setInterval(() => {
            setTime(Date.now());
        }, 42);

        return () => clearInterval(timerId);
    });

    return <>{((time - startTime) / 1000.0).toFixed(2)}</>;
}
