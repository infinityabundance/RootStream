package com.rootstream.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.rootstream.R

/**
 * Status overlay displaying FPS and latency metrics
 */
@Composable
fun StatusOverlay(
    fps: Int,
    latency: Long,
    modifier: Modifier = Modifier
) {
    Column(
        modifier = modifier
            .background(
                color = Color.Black.copy(alpha = 0.7f),
                shape = RoundedCornerShape(8.dp)
            )
            .padding(12.dp)
    ) {
        Text(
            text = stringResource(R.string.stream_fps, fps),
            style = MaterialTheme.typography.bodySmall,
            color = Color.White
        )
        Text(
            text = stringResource(R.string.stream_latency, latency),
            style = MaterialTheme.typography.bodySmall,
            color = Color.White
        )
    }
}
