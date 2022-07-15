export type GamePhase = "intro" | "running" | "end" | "error";

export type GamePlayer = {
    id: number;
    lapsTimesMs: number[];
    finished: boolean;
    totalTimeMs: number;
    winner: boolean;
};

export type UpdateCB = () => void;

export class GameState {
    public phase: GamePhase = "intro";
    public startTimeMs = 0;
    public players: GamePlayer[] = [];

    private listeners: UpdateCB[] = [];

    public static readonly instance = new GameState();
    private constructor() {
        window.ipc.handleHttp((path: string) => {
            const [command, args] = path.substring(1).split('?');
            const qs = new URLSearchParams(args ?? "");
            console.log(command, args);
            switch(command) {
                case 'game_intro':
                    this.phase = "intro";
                    break;
                case 'game_start':
                    this.phase = "running";
                    this.startTimeMs = Date.now();
                    this.players = [];
                    for (let i = 0; i < Number(qs.get('players')); i++) {
                        this.players.push({
                            id: i,
                            finished: false,
                            lapsTimesMs: [],
                            totalTimeMs: 0,
                            winner: false,
                        });
                    }
                    break;
                case 'lap_done': {
                    if (this.phase !== "running") return;
                    const playerN = Number(qs.get('player'));
                    const lap = Number(qs.get('lap')) - 1;
                    const time = Number(qs.get('time'));
                    if (!this.players[playerN]) return;

                    const player = this.players[playerN];
                    while (player.lapsTimesMs.length < lap) {
                        player.lapsTimesMs.push(0);
                    }
                    player.lapsTimesMs[lap] = time;
                    break;
                }
                case 'player_done': {
                    if (this.phase !== "running") return;
                    const playerN = Number(qs.get('player'));
                    const time = Number(qs.get('time'));
                    if (!this.players[playerN]) return;

                    const player = this.players[playerN];
                    player.finished = true;
                    player.totalTimeMs = time;
                    break;
                }
                case 'game_end': {
                    if (this.phase !== "running") return;
                    this.phase = "end";
                    const winner = Number(qs.get('winner'));
                    if (winner < 0) {
                        this.phase = "error";
                    } else {
                        if (!this.players[winner]) return;
                        this.players[winner].winner = true;
                    }
                    break;
                }
                default:
                    return;
            }
            this.update();
        });
    }

    private update() {
        for (const l of this.listeners) l();
    }

    public addListener(cb: UpdateCB) {
        this.listeners.push(cb);
    }

    public removeListener(cb: UpdateCB) {
        const idx = this.listeners.indexOf(cb);
        if (idx < 0) return;
        this.listeners.splice(idx, 1);
    }
}
