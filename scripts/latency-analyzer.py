#!/usr/bin/env python3
"""
latency-analyzer.py - RootStream Latency Analysis Tool

Parses latency log files and generates performance reports.

Usage:
    python3 scripts/latency-analyzer.py [OPTIONS] <logfile>

Options:
    --json          Output as JSON
    --csv           Export raw data as CSV
    --plot          Generate latency plot (requires matplotlib)
    --threshold MS  Warn if p99 exceeds threshold (default: 50ms)

Log Format (from RootStream latency logging):
    LATENCY: capture=XXXus encode=XXXus send=XXXus total=XXXus

Example:
    rootstream host --latency > latency.log
    python3 scripts/latency-analyzer.py latency.log
"""

import sys
import re
import argparse
import json
from dataclasses import dataclass
from typing import List, Optional
from statistics import mean, median, stdev


@dataclass
class LatencySample:
    """Single latency measurement"""
    capture_us: int
    encode_us: int
    send_us: int
    total_us: int

    @property
    def capture_ms(self) -> float:
        return self.capture_us / 1000.0

    @property
    def encode_ms(self) -> float:
        return self.encode_us / 1000.0

    @property
    def send_ms(self) -> float:
        return self.send_us / 1000.0

    @property
    def total_ms(self) -> float:
        return self.total_us / 1000.0


class LatencyAnalyzer:
    """Analyzes RootStream latency logs"""

    # Regex to parse latency lines
    LATENCY_PATTERN = re.compile(
        r'LATENCY:\s*'
        r'capture=(\d+)us\s+'
        r'encode=(\d+)us\s+'
        r'send=(\d+)us\s+'
        r'total=(\d+)us'
    )

    def __init__(self):
        self.samples: List[LatencySample] = []

    def parse_file(self, filename: str) -> int:
        """Parse latency log file. Returns number of samples parsed."""
        count = 0
        with open(filename, 'r') as f:
            for line in f:
                match = self.LATENCY_PATTERN.search(line)
                if match:
                    sample = LatencySample(
                        capture_us=int(match.group(1)),
                        encode_us=int(match.group(2)),
                        send_us=int(match.group(3)),
                        total_us=int(match.group(4))
                    )
                    self.samples.append(sample)
                    count += 1
        return count

    def parse_stdin(self) -> int:
        """Parse latency data from stdin. Returns number of samples parsed."""
        count = 0
        for line in sys.stdin:
            match = self.LATENCY_PATTERN.search(line)
            if match:
                sample = LatencySample(
                    capture_us=int(match.group(1)),
                    encode_us=int(match.group(2)),
                    send_us=int(match.group(3)),
                    total_us=int(match.group(4))
                )
                self.samples.append(sample)
                count += 1
        return count

    def percentile(self, values: List[float], p: float) -> float:
        """Calculate percentile of sorted values"""
        if not values:
            return 0.0
        sorted_values = sorted(values)
        k = (len(sorted_values) - 1) * (p / 100.0)
        f = int(k)
        c = f + 1
        if c >= len(sorted_values):
            return sorted_values[-1]
        return sorted_values[f] + (k - f) * (sorted_values[c] - sorted_values[f])

    def analyze(self) -> dict:
        """Generate analysis report"""
        if not self.samples:
            return {"error": "No samples to analyze"}

        # Extract individual stage timings
        capture_ms = [s.capture_ms for s in self.samples]
        encode_ms = [s.encode_ms for s in self.samples]
        send_ms = [s.send_ms for s in self.samples]
        total_ms = [s.total_ms for s in self.samples]

        def stage_stats(values: List[float], name: str) -> dict:
            return {
                "name": name,
                "count": len(values),
                "min_ms": round(min(values), 2),
                "max_ms": round(max(values), 2),
                "mean_ms": round(mean(values), 2),
                "median_ms": round(median(values), 2),
                "stdev_ms": round(stdev(values), 2) if len(values) > 1 else 0,
                "p50_ms": round(self.percentile(values, 50), 2),
                "p95_ms": round(self.percentile(values, 95), 2),
                "p99_ms": round(self.percentile(values, 99), 2),
            }

        report = {
            "sample_count": len(self.samples),
            "stages": {
                "capture": stage_stats(capture_ms, "Capture"),
                "encode": stage_stats(encode_ms, "Encode"),
                "send": stage_stats(send_ms, "Send"),
                "total": stage_stats(total_ms, "Total"),
            },
            "frame_drops": self.detect_frame_drops(),
            "recommendations": self.generate_recommendations(),
        }

        return report

    def detect_frame_drops(self) -> dict:
        """Detect potential frame drops based on latency spikes"""
        if len(self.samples) < 2:
            return {"detected": 0, "threshold_ms": 0}

        # Frame is considered dropped if total latency > 2x median
        total_ms = [s.total_ms for s in self.samples]
        med = median(total_ms)
        threshold = med * 2

        drops = sum(1 for t in total_ms if t > threshold)
        drop_indices = [i for i, t in enumerate(total_ms) if t > threshold]

        return {
            "detected": drops,
            "threshold_ms": round(threshold, 2),
            "drop_rate_percent": round(100.0 * drops / len(self.samples), 2),
            "drop_indices": drop_indices[:10],  # First 10 only
        }

    def generate_recommendations(self) -> List[str]:
        """Generate performance recommendations"""
        recommendations = []

        if not self.samples:
            return ["No data to analyze"]

        total_ms = [s.total_ms for s in self.samples]
        encode_ms = [s.encode_ms for s in self.samples]
        capture_ms = [s.capture_ms for s in self.samples]

        p99_total = self.percentile(total_ms, 99)
        mean_encode = mean(encode_ms)
        mean_capture = mean(capture_ms)

        # Check total latency
        if p99_total > 50:
            recommendations.append(
                f"p99 latency ({p99_total:.1f}ms) exceeds 50ms target. "
                "Consider reducing resolution or bitrate."
            )
        elif p99_total > 33:
            recommendations.append(
                f"p99 latency ({p99_total:.1f}ms) exceeds 33ms (30fps frame time). "
                "May cause stuttering at high framerates."
            )
        else:
            recommendations.append(
                f"Latency is good (p99: {p99_total:.1f}ms). "
                "Suitable for responsive gameplay."
            )

        # Check encode time
        if mean_encode > 10:
            recommendations.append(
                f"Encode time is high ({mean_encode:.1f}ms avg). "
                "Try enabling low-latency preset or reducing quality."
            )

        # Check capture time
        if mean_capture > 5:
            recommendations.append(
                f"Capture time is high ({mean_capture:.1f}ms avg). "
                "Ensure DRM/KMS capture is working (not software fallback)."
            )

        # Check for high variance
        if len(total_ms) > 10:
            cv = stdev(total_ms) / mean(total_ms)  # Coefficient of variation
            if cv > 0.5:
                recommendations.append(
                    f"High latency variance (CV: {cv:.2f}). "
                    "May indicate system load or thermal throttling."
                )

        return recommendations

    def to_csv(self) -> str:
        """Export samples as CSV"""
        lines = ["capture_us,encode_us,send_us,total_us"]
        for s in self.samples:
            lines.append(f"{s.capture_us},{s.encode_us},{s.send_us},{s.total_us}")
        return "\n".join(lines)

    def print_report(self, report: dict, threshold_ms: float = 50.0):
        """Print formatted report to stdout"""
        print()
        print("╔════════════════════════════════════════════════╗")
        print("║  RootStream Latency Analysis                   ║")
        print("╚════════════════════════════════════════════════╝")
        print()

        print(f"Samples analyzed: {report['sample_count']}")
        print()

        # Stage breakdown table
        print("┌─────────────┬─────────┬─────────┬─────────┬─────────┬─────────┐")
        print("│ Stage       │ Mean    │ Median  │ p95     │ p99     │ Max     │")
        print("├─────────────┼─────────┼─────────┼─────────┼─────────┼─────────┤")

        for stage_name in ["capture", "encode", "send", "total"]:
            stage = report["stages"][stage_name]
            print(f"│ {stage['name']:<11} │ {stage['mean_ms']:>5.1f}ms │ "
                  f"{stage['median_ms']:>5.1f}ms │ {stage['p95_ms']:>5.1f}ms │ "
                  f"{stage['p99_ms']:>5.1f}ms │ {stage['max_ms']:>5.1f}ms │")

        print("└─────────────┴─────────┴─────────┴─────────┴─────────┴─────────┘")
        print()

        # Frame drops
        drops = report["frame_drops"]
        if drops["detected"] > 0:
            print(f"⚠ Frame drops detected: {drops['detected']} "
                  f"({drops['drop_rate_percent']:.1f}%)")
        else:
            print("✓ No frame drops detected")
        print()

        # Recommendations
        print("Recommendations:")
        for rec in report["recommendations"]:
            print(f"  • {rec}")
        print()

        # Threshold check
        p99 = report["stages"]["total"]["p99_ms"]
        if p99 > threshold_ms:
            print(f"⚠ WARNING: p99 latency ({p99:.1f}ms) exceeds "
                  f"threshold ({threshold_ms:.1f}ms)")
            return 1
        else:
            print(f"✓ p99 latency ({p99:.1f}ms) within threshold ({threshold_ms:.1f}ms)")
            return 0


def plot_latency(analyzer: LatencyAnalyzer, output_file: Optional[str] = None):
    """Generate latency plot (requires matplotlib)"""
    try:
        import matplotlib.pyplot as plt
    except ImportError:
        print("ERROR: matplotlib required for plotting. Install with:")
        print("  pip install matplotlib")
        return

    samples = analyzer.samples
    if not samples:
        print("ERROR: No samples to plot")
        return

    # Prepare data
    x = range(len(samples))
    capture = [s.capture_ms for s in samples]
    encode = [s.encode_ms for s in samples]
    send = [s.send_ms for s in samples]
    total = [s.total_ms for s in samples]

    # Create stacked area chart
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))

    # Top: Stacked breakdown
    ax1.stackplot(x, capture, encode, send,
                  labels=['Capture', 'Encode', 'Send'],
                  colors=['#2ecc71', '#3498db', '#e74c3c'],
                  alpha=0.8)
    ax1.set_ylabel('Latency (ms)')
    ax1.set_title('RootStream Latency Breakdown')
    ax1.legend(loc='upper right')
    ax1.grid(True, alpha=0.3)

    # Bottom: Total latency with percentile lines
    ax2.plot(x, total, color='#9b59b6', linewidth=0.5, alpha=0.7)
    ax2.axhline(y=analyzer.percentile(total, 50), color='green',
                linestyle='--', label='p50')
    ax2.axhline(y=analyzer.percentile(total, 95), color='orange',
                linestyle='--', label='p95')
    ax2.axhline(y=analyzer.percentile(total, 99), color='red',
                linestyle='--', label='p99')
    ax2.set_xlabel('Frame')
    ax2.set_ylabel('Total Latency (ms)')
    ax2.set_title('Total Latency with Percentiles')
    ax2.legend(loc='upper right')
    ax2.grid(True, alpha=0.3)

    plt.tight_layout()

    if output_file:
        plt.savefig(output_file, dpi=150)
        print(f"Plot saved to: {output_file}")
    else:
        plt.show()


def main():
    parser = argparse.ArgumentParser(
        description='Analyze RootStream latency logs',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s latency.log                  # Basic analysis
  %(prog)s --json latency.log           # JSON output
  %(prog)s --plot latency.log           # Generate plot
  %(prog)s --threshold 30 latency.log   # Fail if p99 > 30ms
  cat latency.log | %(prog)s -           # Read from stdin
"""
    )

    parser.add_argument('logfile', help='Latency log file (or - for stdin)')
    parser.add_argument('--json', action='store_true', help='Output as JSON')
    parser.add_argument('--csv', action='store_true', help='Export raw data as CSV')
    parser.add_argument('--plot', nargs='?', const=True, default=False,
                        help='Generate plot (optionally specify output file)')
    parser.add_argument('--threshold', type=float, default=50.0,
                        help='p99 threshold in ms (default: 50)')

    args = parser.parse_args()

    analyzer = LatencyAnalyzer()

    # Parse input
    if args.logfile == '-':
        count = analyzer.parse_stdin()
    else:
        try:
            count = analyzer.parse_file(args.logfile)
        except FileNotFoundError:
            print(f"ERROR: File not found: {args.logfile}", file=sys.stderr)
            return 1
        except Exception as e:
            print(f"ERROR: {e}", file=sys.stderr)
            return 1

    if count == 0:
        print("ERROR: No latency samples found in input", file=sys.stderr)
        print("Expected format: LATENCY: capture=XXXus encode=XXXus send=XXXus total=XXXus",
              file=sys.stderr)
        return 1

    # Output modes
    if args.csv:
        print(analyzer.to_csv())
        return 0

    report = analyzer.analyze()

    if args.json:
        print(json.dumps(report, indent=2))
        return 0

    if args.plot:
        output_file = args.plot if isinstance(args.plot, str) else None
        plot_latency(analyzer, output_file)

    return analyzer.print_report(report, args.threshold)


if __name__ == '__main__':
    sys.exit(main() or 0)
