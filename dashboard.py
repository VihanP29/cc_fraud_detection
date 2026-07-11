import os
import subprocess
import sys
import time
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


def run_engine_stream(num_transactions: int, window_size: int, sensitivity: float) -> tuple[float, pd.DataFrame]:
    def execute_stream() -> subprocess.CompletedProcess:
        engine_cmd = [
            locate_engine_executable(),
            "--stream",
            str(num_transactions),
            "--window",
            str(window_size),
            "--sensitivity",
            str(sensitivity),
        ]
        return subprocess.run(
            engine_cmd,
            cwd=ROOT_DIR,
            capture_output=True,
            text=True,
            check=True,
        )

    try:
        completed = execute_stream()
    except subprocess.CalledProcessError as exc:
        stderr = exc.stderr or ""
        if "Unknown argument: --window" in stderr or "Unknown argument: --sensitivity" in stderr:
            compile_engine()
            completed = execute_stream()
        else:
            raise

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
    chart_placeholder = st.empty()
    alert_placeholder = st.empty()
    chunk_size = 50

    for start in range(0, len(df), chunk_size):
        end = min(start + chunk_size, len(df))
        visible_df = df.iloc[:end]

        fig = px.line(
            visible_df,
            x="Transaction_ID",
            y="Amount",
            title="Transaction Amounts Over Time",
            labels={"Transaction_ID": "Transaction ID", "Amount": "Amount ($)"},
        )
        anomalies = visible_df[visible_df["Is_Anomaly"] == 1]
        if not anomalies.empty:
            fig.add_scatter(
                x=anomalies["Transaction_ID"],
                y=anomalies["Amount"],
                mode="markers",
                marker=dict(color="red", size=10, symbol="x"),
                name="Anomaly",
            )
            alert_placeholder.error(
                f"Live Alerts: {len(anomalies)} anomaly(s) detected in transactions {start + 1}-{end}."
            )
        else:
            alert_placeholder.info(
                f"Live Alerts: No anomalies in transactions {start + 1}-{end}."
            )

        fig.update_layout(hovermode="x unified")
        chart_placeholder.plotly_chart(fig, use_container_width=True)
        time.sleep(0.06)

    alert_placeholder.success("Stream complete: audit summary below.")


def render_audit_report(df: pd.DataFrame, tps: float, window_size: int, sensitivity: float) -> None:
    total_transactions = len(df)
    total_anomalies = int(df["Is_Anomaly"].sum())
    max_anomaly_value = float(df.loc[df["Is_Anomaly"] == 1, "Amount"].max()) if total_anomalies else 0.0
    anomaly_rate = (total_anomalies / total_transactions) * 100 if total_transactions else 0.0

    st.markdown("---")
    st.markdown("### Post-Market Audit Report")
    st.markdown(
        f"**Operational Summary:** The C++ risk engine processed {total_transactions:,} transactions using a rolling window of {window_size} and "
        f"a sensitivity threshold of {sensitivity:.2f} standard deviations."
    )
    st.markdown(
        f"- **Total Anomalies Flagged:** {total_anomalies:,} ({anomaly_rate:.2f}% of stream)\n"
        f"- **Peak Anomaly Value:** ${max_anomaly_value:,.2f}\n"
        f"- **Average Throughput:** {tps:,.2f} TPS\n"
    )
    st.markdown(
        "**Risk Narrative:** The system maintained stable throughput while preserving a conservative anomaly threshold. "
        "Any red alerts above indicate deviations that warrant immediate audit and follow-up action."
    )


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
        window_size = st.slider(
            "Window Size",
            min_value=10,
            max_value=100,
            value=50,
            step=5,
        )
        sensitivity = st.slider(
            "Detection Sensitivity (Std Dev)",
            min_value=1.0,
            max_value=5.0,
            value=3.0,
            step=0.1,
        )
        refresh = st.button("Refresh Stream")

    if refresh:
        try:
            with st.spinner("Running C++ engine and loading results..."):
                tps, df = run_engine_stream(transactions, window_size, sensitivity)
            st.metric("Transactions Per Second (TPS)", f"{tps:,.2f}")
            plot_transactions(df)
            render_audit_report(df, tps, window_size, sensitivity)
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
