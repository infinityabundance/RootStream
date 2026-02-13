# VPC Outputs
output "vpc_id" {
  description = "ID of the VPC"
  value       = aws_vpc.rootstream.id
}

output "public_subnet_ids" {
  description = "IDs of public subnets"
  value       = aws_subnet.public[*].id
}

output "private_subnet_ids" {
  description = "IDs of private subnets"
  value       = aws_subnet.private[*].id
}

# EKS Outputs
output "eks_cluster_id" {
  description = "ID of the EKS cluster"
  value       = aws_eks_cluster.rootstream.id
}

output "eks_cluster_endpoint" {
  description = "Endpoint for EKS cluster"
  value       = aws_eks_cluster.rootstream.endpoint
}

output "eks_cluster_security_group_id" {
  description = "Security group ID attached to the EKS cluster"
  value       = aws_eks_cluster.rootstream.vpc_config[0].cluster_security_group_id
}

output "eks_cluster_certificate_authority_data" {
  description = "Base64 encoded certificate data for cluster"
  value       = aws_eks_cluster.rootstream.certificate_authority[0].data
  sensitive   = true
}

# RDS Outputs
output "rds_endpoint" {
  description = "Endpoint of the RDS database"
  value       = aws_db_instance.rootstream.endpoint
}

output "rds_address" {
  description = "Address of the RDS database"
  value       = aws_db_instance.rootstream.address
}

output "rds_port" {
  description = "Port of the RDS database"
  value       = aws_db_instance.rootstream.port
}

output "rds_database_name" {
  description = "Name of the database"
  value       = aws_db_instance.rootstream.db_name
}

# Redis Outputs
output "redis_endpoint" {
  description = "Primary endpoint of the Redis cluster"
  value       = aws_elasticache_replication_group.rootstream.primary_endpoint_address
}

output "redis_port" {
  description = "Port of the Redis cluster"
  value       = aws_elasticache_replication_group.rootstream.port
}

output "redis_reader_endpoint" {
  description = "Reader endpoint of the Redis cluster"
  value       = aws_elasticache_replication_group.rootstream.reader_endpoint_address
}

# Load Balancer Outputs
output "alb_dns_name" {
  description = "DNS name of the Application Load Balancer"
  value       = aws_lb.rootstream.dns_name
}

output "alb_zone_id" {
  description = "Zone ID of the Application Load Balancer"
  value       = aws_lb.rootstream.zone_id
}

# S3 Outputs
output "s3_bucket_name" {
  description = "Name of the S3 bucket"
  value       = aws_s3_bucket.rootstream.id
}

output "s3_bucket_arn" {
  description = "ARN of the S3 bucket"
  value       = aws_s3_bucket.rootstream.arn
}

# ECR Outputs
output "ecr_repository_url" {
  description = "URL of the ECR repository"
  value       = aws_ecr_repository.rootstream.repository_url
}

# CloudWatch Outputs
output "cloudwatch_log_group_name" {
  description = "Name of the CloudWatch log group"
  value       = aws_cloudwatch_log_group.rootstream.name
}

# Connection String (for reference only, use secrets manager in production)
output "database_connection_string" {
  description = "Database connection string (use with caution)"
  value       = "postgresql://${var.db_username}:${var.db_password}@${aws_db_instance.rootstream.endpoint}/${aws_db_instance.rootstream.db_name}"
  sensitive   = true
}

output "redis_connection_string" {
  description = "Redis connection string"
  value       = "redis://${aws_elasticache_replication_group.rootstream.primary_endpoint_address}:${aws_elasticache_replication_group.rootstream.port}"
  sensitive   = true
}
