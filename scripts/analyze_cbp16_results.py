import os
import sys

import pandas as pd
import numpy as np

import matplotlib.pyplot as plt
import seaborn as sns

SCRIPT_DIR = os.path.dirname(__file__)
RES_FNAME = 'cbp2016_all_results.csv'
RES_FPATH = os.path.join(SCRIPT_DIR, RES_FNAME)

# For more natural MultiIndex indexing
# https://pandas.pydata.org/pandas-docs/stable/user_guide/advanced.html#using-slicers
idx = pd.IndexSlice

sns.set(
    context='paper',
    # context='talk',
    font_scale=1.25,
)


# Size of BinDataPoint: 24
#     branchTaken:  1(0x7ffe289e8060)
#     predDir:      1(0x7ffe289e8061)
#     conditional:  1(0x7ffe289e8062)
#     opType:       4(0x7ffe289e8064)
#     branchTarget: 8(0x7ffe289e8068)
#     PC:           8(0x7ffe289e8070)

def sort_func(s):
    """sorts s by string first and then number at end before .res"""
    s = s.rsplit('.', 1)[0]
    s = s.rsplit('-', 1)
    if len(s) == 1:
        return s[0], 0
    s1, s2 = s
    s2 = int(s2) if '(' not in s2 else 0
    return s1, s2


def read_results(result_dir):
    results = []
    for submission in sorted(os.listdir(result_dir)):
        submission_name, submission_type = submission.rsplit('_', 1)
        submission_type = submission_type.replace('cbp', '')
        submission_dir = os.path.join(result_dir, submission)
        for result_file in sorted(os.listdir(submission_dir), key=sort_func):
            if result_file == 'rollup' or result_file.endswith(' (2).res'):
                continue  # ignore these files
            with open(os.path.join(submission_dir, result_file), 'r') as f:
                result = f.read()
            # Edge case 1...
            result = result.replace('Edge sequence list access window '
                                    'overflow!\n\n', ' ')
            # Edge case 2...
            maybe_idx = result.find('Breakdowns:')
            if maybe_idx != -1:
                result = ''.join(result[maybe_idx:].strip().split('\n')[5:])
            else:
                # Edge case 3...
                maybe_idx = result.find('(TOTAL')
                if maybe_idx != -1:
                    maybe_idx = result.find(')', maybe_idx)
                    if maybe_idx == -1:
                        raise RuntimeError('Could not parse the file',
                                           result_file, 'with contents:',
                                           result)
                    result = result[maybe_idx + 1:].strip()
                else:
                    result = result.strip()
            if len(result.split('\n')) > 1:  # Check for unexpected things
                print('=' * 80 + '\n{: >30} - {}'.format(
                    submission, result_file) + '\n' + '-' * 80, file=sys.stderr)
                print(result, file=sys.stderr)
                print('=' * 80 + '\n', file=sys.stderr)
                raise RuntimeError('Error parsing a result. See stderr output.')
            # Put into standard formatting...
            result = result.replace(':', '').split()
            results.append({
                result[i]: result[i + 1]
                # Exclude trace paths
                for i in range(0, len(result), 2) if result[i] != 'TRACE'
            })
            trace_full, trace_n = result_file.rsplit('.', 1)[0].rsplit('-', 1)
            trace_length, trace_sys = trace_full.split('_')
            results[-1].update(name=submission_name, size=submission_type,
                               trace_length=trace_length, trace_sys=trace_sys,
                               trace_n=trace_n)
    df = pd.DataFrame(results)
    df.set_index(['name', 'size', 'trace_length', 'trace_sys', 'trace_n'],
                 inplace=True)
    for key in df.keys():
        if key.startswith('NUM'):
            not_na = ~pd.isna(df[key])
            df.loc[not_na, key] = df.loc[not_na, key].astype(int)
        if key.startswith('MPKBr') or key.startswith('MISPRED'):
            not_na = ~pd.isna(df[key])
            df.loc[not_na, key] = df.loc[not_na, key].astype(float)

    return df


# DATA HEADERS
N_INSTR = 'NUM_INSTRUCTIONS'
N_MPRED = 'NUM_MISPREDICTIONS'
N_BRNCH = 'NUM_BR'
N_CONDB = 'NUM_CONDITIONAL_BR'
# METRIC HEADERS
MPRED1K = 'MISPRED_PER_1K_INST'
MPRED1KB = 'MISPRED_PER_1K_BRAN'
MPRED1KCB = 'MISPRED_PER_1K_COND'
MPREDW1K = 'WEIGHTED_MISPRED_PER_1K_INST'

MAP_SI2MAG = dict(K=3, M=6, B=9)


def main(result_dir, no_cache=False):
    if not no_cache and os.path.exists(RES_FPATH):
        print('Using cached version of CBP-16 results from:', RES_FPATH)
        # 5 arises from the 5 levels of MultiIndex:
        #  ['name', 'size', 'trace_length', 'trace_sys', 'trace_n']
        df_res = pd.read_csv(RES_FPATH, index_col=list(range(5)))
    else:
        print('Parsing results into DataFrame CBP-16')
        df_res = read_results(result_dir)

        # RECOMPUTE MISSES PER 1K INSTRUCTIONS
        df_res.loc[:, MPRED1K + '_IMPRECISE'] = df_res[MPRED1K]
        df_res.loc[:, MPRED1K] = df_res[N_MPRED] / (df_res[N_INSTR] / 1000)

        # MISSES...
        # for all branches
        df_res.loc[:, MPRED1KB] = df_res[N_MPRED] / (df_res[N_BRNCH] / 1000)
        # for conditional branches
        df_res.loc[:, MPRED1KCB] = df_res[N_MPRED] / (df_res[N_CONDB] / 1000)

        df_res.to_csv(RES_FPATH)

    print('Working with index:')
    print(' ' * 3, list(df_res.index.names))
    print('Working with headers:')
    print(' ' * 3, list(df_res.keys()))

    submission_rename = {
        # '2014_AndreSeznec':
        #  'Exploring branch predictability limits with the MTAGE+SC predictor',
        '2014_AndreSeznec': 'MTAGE+SC',
        # 'AndreSeznec': 'TAGE-SC-L Branch Predictors Again',
        'AndreSeznec': 'TAGE-SC-L',
        # 'DanielJimenez1': 'Multiperspective Perceptron Predictor',
        'DanielJimenez1': 'MP-Perceptron',
        # 'DanielJimenez2': 'Multiperspective Perceptron Predictor with TAGE',
        'DanielJimenez2': 'MP-Perceptron-TAGE',
        # 'StephenPruett': 'Dynamically Sizing the TAGE Branch Predictor',
        'StephenPruett': 'Dynamic-TAGE',
    }

    # PLOTS
    if False:
        # Create index of the arbitrarily first submission and submission type
        #  (first two levels of MultiIndex)
        arb_idx = idx[df_res.index.get_level_values(0)[0],
                      df_res.index.get_level_values(1)[0]]
        # Reset index is necessary to transform MultiIndex to columns
        df_trace = df_res.loc[arb_idx, :].reset_index()
        df_trace.replace({'name': submission_rename}, inplace=True)
        # sns.catplot(x='trace_length', y=N_INSTR, hue='trace_sys', kind='violin',
        #             split=True, inner='stick', data=df_trace)
        # sns.catplot(x='trace_length', y=N_INSTR, hue='trace_sys', kind='boxen',
        #             data=df_trace)
        sns.catplot(x='trace_length', y=N_INSTR, hue='trace_sys', kind='box',
                    data=df_trace)
        # plt.yscale('log')
        plt.show()

    # ======== PLOT 1 ========
    # Create index of the arbitrarily first submission and submission type
    #  (first two levels of MultiIndex)
    arb_idx = idx[df_res.index.get_level_values(0)[0],
                  df_res.index.get_level_values(1)[0]]
    df_trace = df_res.loc[arb_idx, :].reset_index()  # creates copy
    df_trace.replace({'name': submission_rename}, inplace=True)
    # instruction type: all, branch, conditional
    # instruction count: int
    # trace_sys
    df_inst_all = df_trace.copy()
    df_inst_br = df_trace.copy()
    df_inst_cb = df_trace.copy()

    df_inst_all.loc[:, 'inst_type'] = 'All'
    df_inst_all.loc[:, 'num_inst'] = df_inst_all[N_INSTR]

    df_inst_br.loc[:, 'inst_type'] = 'Branch'
    df_inst_br.loc[:, 'num_inst'] = df_inst_br[N_BRNCH]

    df_inst_cb.loc[:, 'inst_type'] = 'Conditional'
    df_inst_cb.loc[:, 'num_inst'] = df_inst_cb[N_CONDB]

    df_trace_inst = pd.concat([df_inst_all, df_inst_br, df_inst_cb],
                              ignore_index=True)
    # Cleanup
    del df_inst_all, df_inst_br, df_inst_cb

    g = sns.catplot(x='trace_sys', y='num_inst', hue='inst_type', kind='box',
                    col='trace_length', data=df_trace_inst)
    for ax in g.axes.flatten():
        ax.set_yscale('log')

    plt.show()

    # ======== PLOT 2 ========
    df_res_flat = df_res.reset_index()  # creates copy
    # Remove size 32KB (this only has one entry...)
    drop_idx = df_res_flat[df_res_flat['size'] == '32KB'].index
    df_res_flat.drop(index=drop_idx, inplace=True)
    df_res_flat.reset_index(inplace=True, drop=True)
    df_res_flat.replace({'name': submission_rename}, inplace=True)
    df_miss_all = df_res_flat.copy()

    # TODO: rename names to respective BPU names

    def get_miss_min(scores):
        return min(scores, key=lambda s: s[1])

    def sort_miss(scores):
        return sorted(scores, key=lambda s: s[1])

    for size, df_miss_sz in df_miss_all.groupby('size'):
        scores_all = []
        scores_br = []
        scores_cb = []
        scores_allw = []

        for name, df_miss_sz_n in df_miss_sz.groupby('name'):
            scores_all.append((name, np.mean(df_miss_sz_n[MPRED1K])))
            scores_br.append((name, np.mean(df_miss_sz_n[MPRED1KB])))
            scores_cb.append((name, np.mean(df_miss_sz_n[MPRED1KCB])))
            # weighted
            scores_allw.append(
                (name,
                 df_miss_sz_n[N_MPRED].sum() /
                 (df_miss_sz_n[N_INSTR].sum() / 1000)
                 )
            )

        print('Minimum Scores for size', size)

        print('All Instr', get_miss_min(scores_all))
        for i, score in enumerate(sort_miss(scores_all)):
            print('#{}:'.format(i + 1), score)

        print()
        print('Branch Instr', get_miss_min(scores_br))
        for i, score in enumerate(sort_miss(scores_br)):
            print('#{}:'.format(i + 1), score)
        print()

        print('Conditional Instr', get_miss_min(scores_cb))
        for i, score in enumerate(sort_miss(scores_cb)):
            print('#{}:'.format(i + 1), score)
        print()

        print('All Instr Weighted', get_miss_min(scores_allw))
        for i, score in enumerate(sort_miss(scores_allw)):
            print('#{}:'.format(i + 1), score)
        print()

    df_miss_br = df_miss_all.copy()
    df_miss_cb = df_miss_all.copy()

    df_miss_all.loc[:, 'metric'] = MPRED1K
    df_miss_all.loc[:, 'num_misses'] = df_miss_all[MPRED1K]

    df_miss_br.loc[:, 'metric'] = MPRED1KB
    df_miss_br.loc[:, 'num_misses'] = df_miss_br[MPRED1KB]

    df_miss_cb.loc[:, 'metric'] = MPRED1KCB
    df_miss_cb.loc[:, 'num_misses'] = df_miss_cb[MPRED1KCB]

    df_misses = pd.concat([df_miss_all, df_miss_br, df_miss_cb],
                          ignore_index=True)

    # Column order by size
    col_order = sorted(df_miss_all['size'].unique(),
                       key=lambda s: (s == 'Unl', len(s), s))
    # Cleanup
    del df_miss_all, df_miss_br, df_miss_cb

    g = sns.catplot(x='num_misses', y='name', hue='metric', kind='bar',
                    col='size', col_order=col_order, data=df_misses,
                    orient='h')
    # for ax in g.axes.flat:
    #     ax.set_xscale('log')
    plt.show()

    # PLOT 3....
    # Remove anything but 8KB (this only has one entry...)
    drop_idx = df_res_flat[df_res_flat['size'] != '8KB'].index
    df_res_flat_8kb = df_res_flat.drop(index=drop_idx)
    df_res_flat_8kb.reset_index(inplace=True, drop=True)
    df_res_flat_8kb.replace({'name': submission_rename}, inplace=True)

    PLOT1 = True

    df_res_flat_8kb.loc[:, 'trace_name'] = (
            df_res_flat_8kb['trace_length'] +
            df_res_flat_8kb['trace_sys']
    )

    # Find changes in trace names, use indices of such to plot lines on hists
    #  -1 is present as loc is range inclusive both ends...
    trace_name_s = df_res_flat_8kb.loc[:len(df_trace) - 1, 'trace_name']
    # idx_change = np.where(trace_name_s[:-1].values !=
    #                       trace_name_s[1:].values)[0]
    # before_labels = trace_name_s.loc[idx_change]
    # after_labels = trace_name_s.loc[idx_change + 1]

    idx_change = np.concatenate([[0], np.where(trace_name_s[:-1].values !=
                                               trace_name_s[1:].values)[0]])
    after_labels = trace_name_s.loc[idx_change + 1]

    df_res_flat_8kb.rename(columns={MPRED1K: 'MPKI'}, inplace=True)

    if PLOT1:
        df_res_flat_8kb.loc[:, 'trace_name'] += (
            df_res_flat_8kb['trace_n'].astype(str)
        )
        g = sns.catplot(x='trace_name', y='MPKI', hue='size', kind='bar',
                        row='name', data=df_res_flat_8kb, linewidth=0,
                        legend=False)
    else:
        g = sns.catplot(x='trace_n', y='MPKI', col='trace_name', kind='bar',
                        row='name', data=df_res_flat_8kb, linewidth=0,
                        legend=False)
    g.set_xticklabels([])

    YVAL = 36  # TODO: this is manual

    for ax in g.axes.flat:
        # ax.set_xticklabels(ax.get_xticklabels(), rotation=45,
        #                    horizontalalignment='right')

        # for b, a, xval in zip(before_labels, after_labels, idx_change):
        for a, xval in zip(after_labels, idx_change):
            ax.plot([xval] * 2, [0, YVAL], 'g--', linewidth=1)
            # ax.text(xval - 1, YVAL / 2, b,
            #         verticalalignment='bottom',
            #         horizontalalignment='right', rotation=90, fontsize=9)
            ax.text(xval + 1, YVAL / 2, a,
                    # verticalalignment='bottom',
                    rotation=90,
                    fontsize=9)
    plt.show()
    # ===
    # print(df_res.head(4))


if __name__ == '__main__':
    import argparse

    # noinspection PyTypeChecker
    parser = argparse.ArgumentParser(
        description='Script for analyzing and plotting CBP-16 results',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    default_results_dir = os.path.join(
        SCRIPT_DIR, '..', 'cbp2016.eval', 'results',
        'cbp2016_evaluation_results'
    )
    parser.add_argument('--result_dir', '-d',
                        default=default_results_dir,
                        help='Directory of extracted CBP-16 results')
    parser.add_argument('--no_cache', action='store_true',
                        help='Do not read in a cached version of the CBP-16 '
                             'results - instead re-parse them.')
    args = parser.parse_args()

    main(result_dir=args.result_dir,
         no_cache=args.no_cache)
