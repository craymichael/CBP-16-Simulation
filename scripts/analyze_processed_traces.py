import os
from glob import glob

import pandas as pd
import numpy as np

import matplotlib.pyplot as plt
import seaborn as sns

sns.set(
    context='paper',
    # context='talk',
    font_scale=1.25,
)

RES_PATH = os.path.join(os.path.dirname(__file__), '..',
                        'processed_traces')


def accuracy(df):
    num = df.TP + df.TN
    return num / (num + df.FP + df.FN)


def mpki(df):
    num = df.FP + df.FN
    return 1000 * num / (num + df.TP + df.TN)


def precision(df):
    # taken precision, not taken precision
    return ((df.TP / (df.TP + df.FP)).fillna(0),
            (df.TN / (df.TN + df.FN)).fillna(0))


def recall(df):
    # taken recall, not taken recall
    return ((df.TP / (df.TP + df.FN)).fillna(0),
            (df.TN / (df.TN + df.FP)).fillna(0))


def directionality(df):
    taken = df.TP + df.FN
    not_taken = df.TN + df.FP
    return (taken - not_taken) / (taken + not_taken)


def trans_rate(df):
    taken = df.TP + df.FN
    not_taken = df.TN + df.FP
    return df.trans_count / (taken + not_taken)


def pct_better_than_static(df):
    taken = df.TP + df.FN
    not_taken = df.TN + df.FP
    static_taken = df.directionality >= 0.
    static_acc = np.where(
        static_taken,
        taken / (taken + not_taken),  # static taken
        not_taken / (taken + not_taken)  # static not taken
    )
    return df.accuracy - static_acc


def dynamic_executions(df):
    return df.TP + df.TN + df.FP + df.FN


def plot_dynamic_exec_vs_acc(df_tot):
    # g = sns.relplot(x='dynamic_executions', y='accuracy', data=df_tot,
    #                 kind='scatter', s=10)
    # for ax in g.axes.flat:
    #     # ax.set_xscale('log')
    #     # ax.set_yscale('symlog')
    #     ax.set_xlim(ax.get_xlim()[0] / 25, 2.e7)
    #     # ax.set_ylim(.1, 1.1)
    # plt.show()

    df_hm = df_tot.loc[:, ['dynamic_executions', 'accuracy']].copy()
    df_hm.loc[:, 'dynamic_executions (log10)'] = pd.cut(
        np.log10(df_hm.dynamic_executions),
        100, precision=0
    )
    df_hm.loc[:, 'accuracy'] = pd.cut(100 * df_hm.accuracy, 20, precision=0)
    df_hm = df_hm.groupby(
        ['dynamic_executions (log10)', 'accuracy']
    ).size().reset_index()
    df_hm.rename(columns={0: 'count'}, inplace=True)
    df_hm.loc[:, 'count'] = np.log10(df_hm['count'].values + 1)
    ax = sns.heatmap(df_hm.pivot(index='accuracy',
                                 columns='dynamic_executions (log10)',
                                 values='count'),
                     cbar_kws={'label': 'PC Count (log10)'})
    ax.invert_yaxis()
    plt.tight_layout()
    plt.show()


def plot_trans_rate_vs_acc(df_tot):
    # df_hm = df_tot.loc[:, ['trans_rate', 'directionality', 'accuracy',
    #                        'dynamic_executions']].copy()
    # df_hm.loc[:, 'directionality'] = np.abs(df_hm['directionality'])

    # df_hm.loc[:, 'accuracy'] = np.log10(df_hm.accuracy * df_hm.dynamic_executions + 1e-5)
    # sns.relplot(x='trans_rate', y='directionality', hue='accuracy',
    #             size='dynamic_executions', data=df_hm, palette='coolwarm',
    #             sizes=(5, 200), alpha=.6, linewidth=0)

    # TODO: this uses mpki relative to PC only - try out portion of mpki for
    #  each trace
    df_hm = df_tot.loc[:, ['trans_rate', 'directionality', 'mpki',
                           'dynamic_executions']].copy()
    df_corr = df_hm.copy()
    df_corr.loc[:, 'directionality'] = np.abs(df_corr['directionality'])
    df_corr.loc[:,
    'dyntran'] = df_corr.directionality * df_corr.trans_rate * df_corr.dynamic_executions
    df_corr.loc[:,
    'dynexec'] = df_corr.directionality * df_corr.dynamic_executions
    df_corr.loc[:, 'tranexec'] = df_corr.trans_rate * df_corr.dynamic_executions
    print(df_corr.corr())
    # df_hm.loc[:, 'accuracy'] = 1 - df_hm.accuracy
    # df_hm.loc[:, 'mpki'] = np.log10(df_hm.mpki + 1e-3)
    # df_hm.loc[:, 'mpki'] = np.log10(df_hm.mpki + 1)
    df_hm.loc[:, 'dynamic_executions'] = np.log10(df_hm.dynamic_executions)
    df_hm.sort_values(by=['dynamic_executions'], inplace=True)
    sns.relplot(x='trans_rate', y='directionality',
                hue='dynamic_executions', size='mpki',
                # size='dynamic_executions', hue='mpki',
                data=df_hm, palette='coolwarm',
                sizes=(1, 25), alpha=.6, linewidth=0)
    # plt.tight_layout()
    plt.show()

    # df_hm = df_tot.loc[:, ['trans_rate', 'directionality', 'mpki',
    #                        'dynamic_executions']].copy()
    # sns.pairplot(df_hm)
    # plt.show()

    # return

    df_hm = df_tot.loc[:, ['trans_rate', 'directionality', 'accuracy',
                           'dynamic_executions']].copy()
    df_hm.loc[:, 'trans_rate'] = pd.cut(
        100 * df_hm.trans_rate,
        25, precision=0
    )
    df_hm.loc[:, 'directionality'] = pd.cut(
        # 100 * np.abs(df_hm.directionality),
        100 * df_hm.directionality,
        25, precision=0
    )
    # df_hm.loc[:, 'accuracy'] = np.log10(100 * df_hm.accuracy + .1)
    # df_hm.loc[:, 'accuracy'] = np.log10(100 * df_hm.accuracy * df_hm.dynamic_executions + 1e-3)
    df_hm.loc[:, 'accuracy'] = 100 * df_hm.accuracy
    # df_hm = df_hm.groupby(
    #     ['dynamic_executions (log10)', 'accuracy']
    # ).size().reset_index()
    # df_hm.rename(columns={0: 'count'}, inplace=True)
    # df_hm.loc[:, 'count'] = np.log10(df_hm['count'].values + 1)
    print(df_hm)
    ax = sns.heatmap(pd.pivot_table(df_hm, index='trans_rate',
                                    columns='directionality',
                                    values='accuracy',
                                    aggfunc='mean'),
                     cbar_kws={'label': 'accuracy'})
    ax.invert_yaxis()
    plt.tight_layout()
    plt.show()


def plot_metrics_optype_wise(df_tot):
    df_tot = df_tot.melt(id_vars=ID_NAMES + ['PC', 'opType'],
                         value_vars=METRICS_PLOT,
                         var_name='metric')

    sns.catplot(x='opType', y='value', hue='metric', data=df_tot,
                kind='bar')
    plt.show()
    print(df_tot['opType'].value_counts())


def plot_metrics_pc_wise(df_trs):
    """Trace-wise metrics"""
    g = sns.catplot(x='trace_full', y='value', row='metric',
                    data=df_trs, kind='bar')
    g.set_xticklabels([])
    plt.tight_layout()
    plt.show()


def plot_frac_mispred_per_pc(df_pc_norm):
    g = sns.relplot(  # x='n_instr',
        x='dynamic_executions',
        y='mpki',
        kind='scatter',
        data=df_pc_norm)
    g.ax.set_xscale('log')
    plt.show()

    n = 50
    mpki_per_k = [[] for _ in range(n)]
    for tr, df in df_pc_norm.groupby('trace_full'):
        k = min(n, len(df))
        # k max values of mpki
        m_vals = df.mpki.values
        idxs = np.argpartition(m_vals, -k)[-k:]
        mpki_sort = np.sort(m_vals[idxs])[::-1]
        m_total = np.sum(m_vals)
        for i in range(n):
            mpki_per_k[i].append(sum(mpki_sort[:i + 1]) / m_total)
    pcts = []
    for i in range(n):
        pct = np.mean(mpki_per_k[i]) * 100
        print('{}% of MPKI on average due to top {} H2Ps'.format(
            pct, i + 1))
        pcts.append(pct)
    df = pd.DataFrame({'Average % of MPKI': pcts,
                       'Top k H2Ps': np.arange(1, n + 1)})
    sns.catplot(x='Average % of MPKI', y='Top k H2Ps',
                orient='h', kind='bar',
                data=df)
    plt.show()


ID_NAMES = ['trace_full', 'trace_type', 'trace_n']
METRICS_PLOT = ['accuracy', 'recall_taken', 'recall_not_taken',
                'precision_taken', 'precision_not_taken']


def main():
    """
    Index:   PC
    Columns: TP, FP, TN, FN, opType, trans_count

    Index:   'warmup_{}pct'.format(int(100 * wp))
    """
    if not os.path.exists(RES_PATH):
        raise RuntimeError(
            'Expected the directory {} to exist. You must run the '
            'process_traces.py script before analyzing results on the '
            'processed traces.'.format(RES_PATH)
        )
    print('Looking at results from', RES_PATH)

    dfs = []
    ss = []

    to_process = glob(os.path.join(RES_PATH, '*.h5'))
    # to_maybe_process = glob(os.path.join(RES_PATH, '*_warmup.h5'))
    #
    # messup_mode = bool(to_maybe_process)
    # if messup_mode:
    #     assert len(to_process) == (len(to_maybe_process) * 2)

    for filename in to_process:
        trace_name = os.path.basename(filename).rsplit('.', 1)[0]
        trace_type, trace_n = trace_name.split('_', 1)[1].split('-')
        # if not (messup_mode and filename.endswith('_warmup.h5')):
        # DataFrame of results
        df = pd.read_hdf(filename, 'df')
        # Finalize df
        df.reset_index(inplace=True)
        df.loc[:, 'trace_full'] = trace_name
        df.loc[:, 'trace_type'] = trace_type
        df.loc[:, 'trace_n'] = int(trace_n)
        dfs.append(df)
        # if not messup_mode or filename.endswith('_warmup.h5'):
        # Series of warmup results
        s = pd.read_hdf(filename, 's')
        s['trace_full'] = trace_name
        ss.append(s)
    df_tot = pd.concat(dfs, ignore_index=True)
    df_warmup = pd.DataFrame(ss)

    metric_names = []

    # Augment with PC-wise aggregate metrics
    for metric in [accuracy,
                   mpki,
                   directionality,
                   dynamic_executions,
                   trans_rate]:
        m_name = metric.__name__
        metric_names.append(m_name)
        df_tot.loc[:, m_name] = metric(df_tot)
    # Augment with PC-wise class-specific metrics
    for metric in [precision,
                   recall]:
        m_res_1, m_res_0 = metric(df_tot)
        m_name1 = metric.__name__ + '_taken'
        m_name0 = metric.__name__ + '_not_taken'
        metric_names.append(m_name1)
        metric_names.append(m_name0)
        df_tot.loc[:, m_name1] = m_res_1
        df_tot.loc[:, m_name0] = m_res_0
    # Misc. metrics that need computation after these
    for metric in [pct_better_than_static]:
        m_name = metric.__name__
        metric_names.append(m_name)
        df_tot.loc[:, m_name] = metric(df_tot)

    # Plots
    for plot_func in [
        # TODO: uncomment what you want plotted
        plot_dynamic_exec_vs_acc,
        plot_trans_rate_vs_acc,
        plot_metrics_optype_wise,
    ]:
        plot_func(df_tot)

    # print(df_tot.groupby('trace_full').agg('mean'))

    # TODO: melt each metric, plot grouped by trace, by opType (PC-wise)
    # TODO: also, recompute each metric weighed by total instructions per trace
    # TODO: bar plot accuracy - accuracy with static branch prediction if better

    # Compute trace-wise metrics
    df_pc_norm = []
    df_trs = []
    for ids, df_tr in df_tot.groupby(ID_NAMES):
        d_pc_norm = dict(zip(ID_NAMES, ids))
        d_tr = d_pc_norm.copy()
        d_pc_norm['PC'] = df_tr.PC
        d_pc_norm['opType'] = df_tr.opType

        n_instr = np.sum(df_tr.dynamic_executions.values)
        d_tr['n_instr'] = n_instr
        for m_name in metric_names:
            # Normalize metrics...
            m_weighted = (df_tr[m_name] * df_tr.dynamic_executions
                          if m_name != 'dynamic_executions' else df_tr[m_name])
            # ...PC-wise normalized (sum of metric entries is total for that
            # metric)
            d_pc_norm[m_name] = m_weighted / n_instr
            # ...for the whole trace
            d_tr[m_name] = np.sum(m_weighted.values) / n_instr
        df_pc_norm.append(pd.DataFrame(d_pc_norm))
        df_trs.append(pd.DataFrame([d_tr]))

    df_pc_norm = pd.concat(df_pc_norm, ignore_index=True)
    # Plots using normalized metrics (% PC contributed to misses for that trace)
    # NOTE: dynamic_executions per PC will be normed, but n_instr won't be (same
    # stat basically)
    for plot_func in [
        plot_frac_mispred_per_pc,
    ]:
        plot_func(df_pc_norm)

    df_trs = pd.concat(df_trs, ignore_index=True)

    # NOTE: sorting needed to align trace names
    df_warmup = df_warmup.sort_values(by=['trace_full'], ignore_index=True)
    df_trs = df_trs.sort_values(by=['trace_full'], ignore_index=True)

    # Unpivot (wide to long)
    df_trs_long = df_trs.melt(id_vars=ID_NAMES,
                              value_vars=METRICS_PLOT,
                              var_name='metric')

    for plot_func in [
        plot_metrics_pc_wise,
    ]:
        plot_func(df_trs_long)

    # df_pc_norm = df_pc_norm.melt(id_vars=ID_NAMES + ['PC', 'opType'],
    #                              value_vars=METRICS_PLOT,
    #                              var_name='metric')

    # sns.relplot(x='PC', y='mpki', data=df_pc_norm)

    # TODO: WIP here
    # plt.scatter(range(len(df_pc_norm)), df_pc_norm.mpki)
    # plt.show()

    warm_keys = [k for k in df_warmup.keys() if k.startswith('warm')]
    warm_keys_norm = []
    for k in warm_keys:
        k_norm = k + '_norm'
        warm_keys_norm.append(k_norm)
        # df_warmup.loc[:, k_norm] = df_warmup[k] / df_trs.n_instr
        df_warmup.loc[:, k_norm] = \
            np.minimum(df_warmup[k], df_trs.n_instr * (1. - df_trs.accuracy)) \
            / df_trs.n_instr

    if True:
        df_warmup_long = df_warmup.melt(id_vars=['trace_full'],
                                        value_vars=warm_keys,
                                        var_name='warmup portion')
        g = sns.catplot(x='trace_full', y='value', row='warmup portion',
                        data=df_warmup_long, kind='bar', linewidth=0)
        for ax in g.axes.flat:
            ax.set_yscale('log')

        #####
        df_warmup_norm_long = df_warmup.melt(id_vars=['trace_full'],
                                             value_vars=warm_keys_norm,
                                             var_name='warmup portion normed')
        g = sns.catplot(x='trace_full', y='value', row='warmup portion normed',
                        data=df_warmup_norm_long, kind='bar', linewidth=0)
        for ax in g.axes.flat:
            ax.set_yscale('log')
        plt.show()

    for k in warm_keys:
        print(k, np.corrcoef(
            np.minimum(df_warmup[k], df_trs.n_instr * (1. - df_trs.accuracy)),
            df_trs.mpki)[0, 1])
    for k in warm_keys_norm:
        print(k, np.corrcoef(df_warmup[k], df_trs.mpki)[0, 1])

    print('Percentage of misses attributed to warmup...')
    # df_warmup.to_csv('test.csv')
    for k in warm_keys:
        # parse pct from key
        pct = int(k.split('_')[-1][:-len('pct')]) / 100.
        # pct_acc = df_trs.accuracy * pct
        pct_acc = pct
        # Number missed due to warmup phase
        missed_warmup = df_warmup[k] * (1. - pct_acc)
        # Number missed total
        missed_tot = df_trs.n_instr * (1. - df_trs.accuracy)
        # Proportion missed due to warmup phase
        # NOTE: 1 is cap as some might not achieve e.g. 99% acc...
        miss_warm_prop = np.minimum(missed_warmup / missed_tot, 1.)
        # print('...' + k + ' is ', miss_warm_prop)
        # plt.scatter(range(len(miss_warm_prop)), miss_warm_prop)
        # plt.show()
    # print()
    # print(df_warmup['trace_full'])
    # print(df_trs['trace_full'])

    # g = sns.catplot(x='trace_full', y='weighted_pct_better_than_static',
    #                 data=df_tot, kind='boxen')
    # g.ax.set_yscale('symlog')
    # print(df_trs['pct_better_than_static'].min())
    # ax = sns.barplot(x='trace_full', y='pct_better_than_static',
    #                  data=df_trs)
    # plt.scatter(range(len(df_tot)), df_tot.pct_better_than_static)
    # plt.tight_layout()
    # plt.show()


if __name__ == '__main__':
    main()
