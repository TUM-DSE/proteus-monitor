import sys
import numpy as np
import pandas as pd
from plot import (
    apply_aliases,
    catplot,
    column_alias,
    explode,
    sns,
    PAPER_MODE,
    plt,
    mpl,
    format,
    magnitude_formatter,
    change_width,
    apply_hatch,
    apply_hatch2,
    apply_hatch_ax,
    ROW_ALIASES,
    COLUMN_ALIASES, 
    FORMATTER
)

csvfile_thrp = sys.argv[1]
# csvfile_clk = sys.argv[2]
df_thrp = pd.read_csv(csvfile_thrp, sep=", ")
# df_clk  = pd.read_csv(csvfile_clk, sep=", ")

# df['runtime_ms'] = df['computation_time'] / int(1e6)
# df.replace('c0', 'sha3', inplace=True)
# df.replace('c4', 'nw', inplace=True)

print("vadd: " + str(df_thrp.query('platform=="U50-HBM" and application.str.startswith("vadd")')['number'].mean()))
print("vadd: " + str(df_thrp.query('platform=="U280-HBM" and application.str.startswith("vadd")')['number'].mean()))
print("vadd: " + str(df_thrp.query('platform=="U280-DDR" and application.str.startswith("vadd")')['number'].mean()))
print("stream_io: " + str(df_thrp.query('platform=="U50-HBM" and application.str.startswith("stream_io")')['number'].mean()))
print("stream_io: " + str(df_thrp.query('platform=="U280-HBM" and application.str.startswith("stream_io")')['number'].mean()))
print("stream_io: " + str(df_thrp.query('platform=="U280-DDR" and application.str.startswith("stream_io")')['number'].mean()))

def simple_bars(df: pd.DataFrame, ylabel_name):
    width = 3.3
    #aspect = 1.2
    aspect = 1.4

    g = catplot(
        data=df,
        x="application",
        y="number",
        hue="platform",
        kind="bar", 
        ci="sd",  # show standard deviation! otherwise with_stddev_to_long_form does not work.
        height=width/aspect,
        aspect=aspect,
        palette="pastel",
        orient="v",
        saturation=1
    )
    g.ax.set_xlabel("")
    g.ax.set_ylabel(ylabel_name)
    #g.set(xscale='log')
    #g.ax.set_yticklabels(["Re-use", "Reconfig"])
    hatches = ["", ".."]
    apply_hatch(g, patch_legend=False, hatch_list=hatches)

    FONT_SIZE = 9
    g.ax.annotate(
        "\u2191 Higher is better",
        xycoords="axes fraction",
        xy=(0.2, 1),
        xytext=(0.2, 1),
        fontsize=FONT_SIZE,
        color="navy",
        weight="bold",
    )

    # g.ax.annotate(
    #     "< 1",
    #     xycoords="axes points",
    #     xy=(73, 8),
    #     xytext=(73, 8),
    #     fontsize=FONT_SIZE-1,
    #     color="black",
    # )
    # g.ax.annotate(
    #     "< 2",
    #     xycoords="axes points",
    #     xy=(117, 8),
    #     xytext=(117, 8),
    #     fontsize=FONT_SIZE-1,
    #     color="black",
    # )
    # g.ax.annotate(
    #     "< 1 ms",
    #     xycoords="axes points",
    #     xy=(165, 8),
    #     xytext=(165, 8),
    #     fontsize=FONT_SIZE-1,
    #     color="black",
    # )

    g.ax.legend(prop={'size': 7}, loc='right', bbox_to_anchor=(0.44,0.8))
    g._legend.set(visible=False)

    g.despine()
    return g


graph_thrp = simple_bars(df_thrp, "Throughput [MB/s]")
# graph_clk  = simple_bars(df_clk, "Clock frequency [MHz]")

#mean_values = df.groupby(by=['application','platform'])['runtime_ms'].mean()
#for i, mean_val in enumerate(mean_values):
#    graph.ax.text(i, mean_val, f'{mean_val:.2f}', ha='center', va='bottom')


graph_thrp.savefig("hetero_thrp.pdf", bbox_inches='tight')
# graph_clk.savefig("hetero_clk.pdf", bbox_inches='tight')
