import { GameState } from "../models/GameState";
import { useEffect, useRef } from "react";

export function useGameState(
    handler: (state: GameState) => void,
) {
    const savedHandler = useRef(handler);
    useEffect(() => {
        savedHandler.current = handler;
    }, [handler]);
    useEffect(() => {
        const fn = () => {
            savedHandler.current(GameState.instance);
        };
        GameState.instance.addListener(fn);
        return () => {
            GameState.instance.removeListener(fn);
        };
    });
}
