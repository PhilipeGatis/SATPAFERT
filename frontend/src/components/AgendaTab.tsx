import { useState } from 'react';
import { api, type AQStatus } from '../App';

export default function AgendaTab({ status }: { status: AQStatus | null }) {
    const [d, setD] = useState('0');
    const [h, setH] = useState('');
    const [m, setM] = useState('');

    const handleSave = () => {
        if (h && m) {
            api('POST', '/api/schedule', { tpaDay: +d, tpaHour: +h, tpaMinute: +m });
        }
    };

    const days = ['Domingo', 'Segunda', 'Terça', 'Quarta', 'Quinta', 'Sexta', 'Sábado'];

    return (
        <div className="flex flex-col gap-4 pb-4">
            {/* TPA SCHEDULING CARD */}
            <div className="rounded-2xl bg-card p-5 shadow-md">
                <h2 className="mb-4 text-base font-medium tracking-wide text-text/90 uppercase">TPA Semanal Automática</h2>

                <div className="mb-5 flex items-center justify-between rounded-lg bg-black/20 p-4">
                    <span className="text-sm font-medium text-muted uppercase">Agendamento Atual</span>
                    <span className="font-mono text-lg font-bold text-accent">
                        {status?.tpaDay !== undefined ? `${days[status.tpaDay]} ${String(status.tpaHour).padStart(2, '0')}:${String(status.tpaMinute).padStart(2, '0')}` : '--'}
                    </span>
                </div>

                <div className="flex flex-col gap-4">
                    <label className="text-xs font-bold text-muted uppercase tracking-wider">Dia da Semana</label>
                    <select
                        className="w-full rounded-t-md border-b-2 border-muted bg-white/5 px-3 py-2 text-sm text-text outline-none transition-colors focus:border-accent"
                        value={d}
                        onChange={e => setD(e.target.value)}
                    >
                        {days.map((day, i) => <option key={i} value={i} className="bg-card">{day}</option>)}
                    </select>

                    <div className="flex gap-4">
                        <div className="flex-1 flex flex-col gap-1">
                            <label className="text-xs font-bold text-muted uppercase tracking-wider">Hora</label>
                            <input
                                type="number" min="0" max="23" placeholder="HH"
                                className="w-full rounded-t-md border-b-2 border-muted bg-white/5 px-3 py-2 text-center text-sm text-text outline-none transition-colors focus:border-accent"
                                value={h} onChange={e => setH(e.target.value)}
                            />
                        </div>
                        <div className="flex items-center text-muted font-bold text-xl mt-5">:</div>
                        <div className="flex-1 flex flex-col gap-1">
                            <label className="text-xs font-bold text-muted uppercase tracking-wider">Mino</label>
                            <input
                                type="number" min="0" max="59" placeholder="MM"
                                className="w-full rounded-t-md border-b-2 border-muted bg-white/5 px-3 py-2 text-center text-sm text-text outline-none transition-colors focus:border-accent"
                                value={m} onChange={e => setM(e.target.value)}
                            />
                        </div>
                    </div>

                    <button
                        onClick={handleSave}
                        className="mt-4 rounded-full bg-accent px-6 py-2.5 text-sm font-bold uppercase tracking-wider text-black shadow-md transition-all hover:bg-blue-300 active:scale-95"
                    >
                        Salvar Horário
                    </button>
                </div>
            </div>

        </div>
    );
}
