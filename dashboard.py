import os
import subprocess
import sys
from pathlib import Path
import io

import pandas as pd
import plotly.express as px
import streamlit as st

ROOT_DIR = Path(__file__).resolve().parent


def _find_engine_executable() -> str:
    engine_name = "engine.exe" if sys.platform.startswith("win") else "engine"
    candidate_paths = [
        ROOT_DIR / "build" / "Release" / engine_name,
        ROOT_DIR / "build" / engine_name,
        ROOT_DIR / engine_name,
    ]

    for path in candidate_paths:
        if path.exists():
            return str(path)
    return ""


def compile_engine() -> None:
    build_dir = ROOT_DIR / "build"
    build_dir.mkdir(exist_ok=True)

    try:
        subprocess.run(
            ["cmake", ".."],
            cwd=str(build_dir),
            capture_output=True,
            text=True,
            check=True,
        )
        subprocess.run(
            ["cmake", "--build", ".", "--config", "Release"],
            cwd=str(build_dir),
            capture_output=True,
            text=True,
            check=True,
        )
    except subprocess.CalledProcessError as exc:
        raise RuntimeError(
            "Failed to compile the C++ engine. "
            f"Stdout:\n{exc.stdout}\nStderr:\n{exc.stderr}"
        )


def locate_engine_executable() -> str:
    path = _find_engine_executable()
    if path:
        return path

    compile_engine()
    path = _find_engine_executable()
    if path:
        return path

    raise FileNotFoundError(
        "Engine executable not found after attempted compilation. "
        "Please verify CMake is installed and the C++ code is buildable."
    )


def run_engine_stream(num_transactions: int) -> tuple[float, pd.DataFrame]:
    engine_cmd = [locate_engine_executable(), "--stream", str(num_transactions)]
    completed = subprocess.run(
        engine_cmd,
        cwd=ROOT_DIR,
        capture_output=True,
        text=True,
        check=True,
    )
    output = completed.stdout.strip().splitlines()
    if not output:
        raise RuntimeError("No output received from engine.")
    first_line = output[0].strip().split(",")
    if len(first_line) != 2 or first_line[0] != "TPS":
        raise RuntimeError(f"Unexpected engine header: {output[0]}")
    tps = float(first_line[1])
    csv_text = "\n".join(output[1:])
    df = pd.read_csv(io.StringIO(csv_text))
    return tps, df


def plot_transactions(df: pd.DataFrame) -> None:
    fig = px.line(
        df,
        x="Transaction_ID",
        y="Amount",
        title="Transaction Amounts Over Time",
        labels={"Transaction_ID": "Transaction ID", "Amount": "Amount ($)"},
    )
    anomalies = df[df["Is_Anomaly"] == 1]
    if not anomalies.empty:
        fig.add_scatter(
            x=anomalies["Transaction_ID"],
            y=anomalies["Amount"],
            mode="markers",
            marker=dict(color="red", size=10, symbol="x"),
            name="Anomaly",
        )
    fig.update_layout(hovermode="x unified")
    st.plotly_chart(fig, use_container_width=True)


def main() -> None:
    st.set_page_config(page_title="Fraud Engine Dashboard", layout="wide")
    st.title("C++ Anomaly Detection Dashboard")

    with st.sidebar:
        st.header("Stream Settings")
        transactions = st.number_input(
            "Number of transactions",
            min_value=100,
            max_value=200000,
            value=5000,
            step=100,
        )
        refresh = st.button("Refresh Stream")

    if refresh:
        try:
            with st.spinner("Running C++ engine and loading results..."):
                tps, df = run_engine_stream(transactions)
            st.metric("Transactions Per Second (TPS)", f"{tps:,.2f}")
            plot_transactions(df)
            st.markdown("### Transaction table")
            st.dataframe(df)
        except subprocess.CalledProcessError as exc:
            st.error(f"Engine failed: {exc.stderr}")
        except Exception as exc:
            st.error(str(exc))
    else:
        st.info("Click Refresh Stream to execute the C++ engine and visualize the latest transaction stream.")


if __name__ == "__main__":
    main()
