import type { AQStatus } from '../App';

function Badge({ label, on, texts }: { label: string; on?: boolean; texts: [string, string] }) {
    if (on === undefined) return null;
    return (
        <div className="flex items-center justify-between border-b border-border/50 py-3 last:border-0">
            <span className="text-sm font-medium tracking-wide text-muted">{label}</span>
            <span className={`rounded-full px-3 py-1 text-xs font-bold tracking-wider ${on ? 'bg-accent/20 text-accent' : 'bg-white/5 text-muted'}`}>
                {on ? texts[0] : texts[1]}
            </span>
        </div>
    );
}

export default function HomeTab({ status }: { status: AQStatus | null }) {
    const shortDays = ['D', 'S', 'T', 'Q', 'Q', 'S', 'S'];

    const renderFertTable = () => {
        if (!status?.stocks) return <div className="text-xs text-muted">Aguardando dados...</div>;

        // Find stocks that have at least one dose > 0, excluding Prime (index 4)
        const activeStocks = status.stocks
            .map((s, idx) => ({ ...s, originalIndex: idx }))
            .filter(s => s.originalIndex !== 4 && s.doses?.some(d => Number(d) > 0));

        if (activeStocks.length === 0) {
            return (
                <div className="text-xs font-semibold text-muted italic mt-1">Nenhum agendamento ativo na semana.</div>
            );
        }

        return (
            <div className="overflow-x-auto mt-2 rounded bg-black/20 p-2">
                <table className="w-full text-left border-collapse">
                    <thead>
                        <tr className="border-b border-white/10 text-[10px] text-muted uppercase tracking-wider">
                            <th className="py-2 pr-2 font-medium">Canal</th>
                            <th className="py-2 px-1 font-medium text-center">Hora</th>
                            {shortDays.map((d, i) => (
                                <th key={i} className="py-2 px-1 font-medium text-center">{d}</th>
                            ))}
                        </tr>
                    </thead>
                    <tbody className="text-xs">
                        {activeStocks.map((s, i) => (
                            <tr key={i} className="border-b border-white/5 last:border-0">
                                <td className="py-3 pr-2 font-medium text-text whitespace-nowrap">
                                    {s.name || `CH ${s.originalIndex + 1}`}
                                </td>
                                <td className="py-3 px-1 font-mono text-muted text-center tracking-tighter">
                                    {String(s.sH || 0).padStart(2, '0')}:{String(s.sM || 0).padStart(2, '0')}
                                </td>
                                {[0, 1, 2, 3, 4, 5, 6].map(di => {
                                    const vol = s.doses?.[di] || 0;
                                    return (
                                        <td key={di} className="py-3 px-1 text-center">
                                            {Number(vol) > 0 ? (
                                                <span className="font-bold text-accent2">{Number(vol)}</span>
                                            ) : (
                                                <span className="text-muted/30">-</span>
                                            )}
                                        </td>
                                    );
                                })}
                            </tr>
                        ))}
                    </tbody>
                </table>
            </div>
        );
    };

    return (
        <div className="flex flex-col gap-4">
            {/* Sensors Card */}
            <div className="rounded-2xl bg-card p-5 shadow-md">
                <h2 className="mb-4 text-base font-medium tracking-wide text-text/90 uppercase">Sensores e Segurança</h2>

                <div className="mb-5">
                    {(() => {
                        const wl = status?.waterLevel || 0;
                        const refCm = status?.aqHeight || 20;
                        const pct = status?.optical ? 100 : Math.max(0, Math.min(100, Math.round((1 - wl / refCm) * 100)));
                        const color = pct < 25 ? 'text-danger' : pct < 50 ? 'text-warn' : 'text-accent2';
                        const barColor = pct < 25 ? 'bg-danger' : pct < 50 ? 'bg-warn' : 'bg-accent2';
                        return (
                            <>
                                <div className="mb-2 flex justify-between text-sm font-medium">
                                    <span className="text-muted tracking-wide">Nível de Água</span>
                                    <span className={`font-mono text-lg ${color}`}>
                                        {status ? `${pct}%` : '--'}
                                    </span>
                                </div>
                                <div className="h-2 w-full overflow-hidden rounded-full bg-white/5">
                                    <div
                                        className={`h-full transition-all duration-500 ease-out ${barColor}`}
                                        style={{ width: `${pct}%` }}
                                    />
                                </div>
                            </>
                        );
                    })()}
                </div>

                <div className="flex flex-col">
                    <Badge label="Nível Ótico" on={status?.optical} texts={['CHEIO', 'NORMAL']} />
                    <Badge label="Bóia de Reposição" on={status?.float} texts={['VAZIO', 'NO NÍVEL']} />
                    <Badge label="Bomba Canister" on={status?.canister} texts={['LIGADA', 'DESL.']} />
                </div>
            </div>

            {/* Routine State Card */}
            <div className="rounded-2xl bg-card p-5 shadow-md">
                <h2 className="mb-4 text-base font-medium tracking-wide text-text/90 uppercase">Estado do Sistema</h2>
                <div className="flex flex-col">
                    <Badge label="Estado TPA" on={status?.tpaState !== 'IDLE'} texts={[status?.tpaState || '', 'IDLE']} />
                    <Badge label="Modo Manutenção" on={status?.maintenance} texts={['ATIVO', 'INATIVO']} />
                </div>
            </div>

            {/* TPA SCHEDULE SUMMARY */}
            <div className="rounded-2xl bg-card p-5 shadow-md">
                <h2 className="mb-4 text-base font-medium tracking-wide text-text/90 uppercase">Agendamento TPA</h2>
                {status && !status.tpaConfigReady && (
                    <div className="rounded-lg bg-warn/10 border border-warn/30 px-4 py-3 mb-4">
                        <span className="text-xs font-bold text-warn">⚠ Configuração incompleta — TPA desativada</span>
                    </div>
                )}
                {status?.tpaInterval ? (() => {
                    const lastRunDate = status.tpaLastRun ? new Date(status.tpaLastRun * 1000) : null;
                    const nextRunDate = lastRunDate
                        ? new Date(lastRunDate.getTime() + status.tpaInterval * 86400000)
                        : null;
                    const now = new Date();
                    const daysUntil = nextRunDate ? Math.ceil((nextRunDate.getTime() - now.getTime()) / 86400000) : null;
                    const formatDate = (d: Date) => d.toLocaleDateString('pt-BR', { day: '2-digit', month: '2-digit', year: 'numeric' });

                    return (
                        <div className="flex flex-col gap-3">
                            <div className="flex items-center justify-between border-b border-border/50 py-2">
                                <span className="text-sm text-muted">Volume</span>
                                <span className="font-mono text-sm font-bold text-accent">
                                    {status.tpaPercent}%{status.aquariumVolume ? ` (${(status.aquariumVolume * status.tpaPercent / 100).toFixed(1)} L)` : ''}
                                </span>
                            </div>
                            <div className="flex items-center justify-between border-b border-border/50 py-2">
                                <span className="text-sm text-muted">Intervalo</span>
                                <span className="font-mono text-sm font-bold text-text">a cada {status.tpaInterval} dia{status.tpaInterval > 1 ? 's' : ''}</span>
                            </div>
                            <div className="flex items-center justify-between border-b border-border/50 py-2">
                                <span className="text-sm text-muted">Horário</span>
                                <span className="font-mono text-sm font-bold text-text">{String(status.tpaHour).padStart(2, '0')}:{String(status.tpaMinute).padStart(2, '0')}</span>
                            </div>
                            <div className="flex items-center justify-between border-b border-border/50 py-2">
                                <span className="text-sm text-muted">Última execução</span>
                                <span className="font-mono text-xs text-muted">{lastRunDate ? formatDate(lastRunDate) : 'nunca'}</span>
                            </div>
                            <div className="flex items-center justify-between py-2">
                                <span className="text-sm text-muted">Próxima TPA</span>
                                <span className={`font-mono text-sm font-bold ${daysUntil !== null && daysUntil <= 1 ? 'text-warn' : 'text-accent2'}`}>
                                    {nextRunDate ? `${formatDate(nextRunDate)} (${daysUntil === 0 ? 'HOJE' : daysUntil === 1 ? 'amanhã' : `em ${daysUntil} dias`})` : '--'}
                                </span>
                            </div>
                        </div>
                    );
                })() : (
                    <div className="text-xs text-muted italic">Nenhum agendamento configurado.</div>
                )}
            </div>

            {/* FERTILIZERS WEEKLY VIEW */}
            <div className="rounded-2xl bg-card p-5 shadow-md">
                <div className="mb-4 flex items-center justify-between">
                    <h2 className="text-base font-medium tracking-wide text-text/90 uppercase">Tabela de Fertilizantes</h2>
                    <span className="text-[10px] font-bold text-accent2 uppercase tracking-wider bg-accent2/10 px-2 py-1 rounded-full border border-accent2/20">Em mL</span>
                </div>
                {renderFertTable()}
            </div>
        </div>
    );
}
